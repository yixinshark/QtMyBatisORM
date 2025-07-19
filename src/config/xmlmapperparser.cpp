#include "QtMyBatisORM/xmlmapperparser.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QResource>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>

namespace QtMyBatisORM {

XMLMapperParser::XMLMapperParser(QObject* parent)
    : QObject(parent)
{
}

QList<MapperConfig> XMLMapperParser::parseMappers(const QStringList& resourcePaths)
{
    QList<MapperConfig> mappers;
    
    for (const QString& path : resourcePaths) {
        try {
            MapperConfig config = parseMapper(path);
            mappers.append(config);
        } catch (const QtMyBatisException& e) {
            throw ConfigurationException(
                QStringLiteral("Failed to parse mapper %1: %2").arg(path, e.message())
            );
        }
    }
    
    // 检查跨文件的语句ID重复
    checkForDuplicateStatementIds(mappers);
    
    return mappers;
}

MapperConfig XMLMapperParser::parseMapper(const QString& resourcePath)
{
    QString xmlContent = readResourceFile(resourcePath);
    
    QDomDocument doc;
    
    auto parseResult = doc.setContent(xmlContent);
    if (!parseResult) {
        throw ConfigurationException(
            QStringLiteral("XML parse error in %1 at line %2, column %3: %4")
            .arg(resourcePath)
            .arg(parseResult.errorLine)
            .arg(parseResult.errorColumn)
            .arg(parseResult.errorMessage)
        );
    }
    
    MapperConfig config = parseMapperFromDocument(doc, resourcePath);
    validateMapper(config);
    
    return config;
}

MapperConfig XMLMapperParser::parseMapperFromDocument(const QDomDocument& doc, const QString& xmlPath)
{
    MapperConfig config;
    config.xmlPath = xmlPath;
    
    QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("mapper")) {
        throw ConfigurationException(
            QStringLiteral("Root element must be 'mapper' in %1").arg(xmlPath)
        );
    }
    
    config.namespace_ = root.attribute(QStringLiteral("namespace"));
    if (config.namespace_.isEmpty()) {
        throw ConfigurationException(
            QStringLiteral("Mapper namespace is required in %1").arg(xmlPath)
        );
    }
    
    // 解析resultMap
    QDomNodeList resultMaps = root.elementsByTagName(QStringLiteral("resultMap"));
    for (int i = 0; i < resultMaps.count(); ++i) {
        QDomElement resultMap = resultMaps.at(i).toElement();
        QString id = resultMap.attribute(QStringLiteral("id"));
        QString type = resultMap.attribute(QStringLiteral("type"));
        if (!id.isEmpty()) {
            config.resultMaps[id] = type;
        }
    }
    
    // 解析SQL语句
    QStringList statementTags = {QStringLiteral("select"), QStringLiteral("insert"), QStringLiteral("update"), QStringLiteral("delete"), QStringLiteral("sql")};
    for (const QString& tag : statementTags) {
        QDomNodeList statements = root.elementsByTagName(tag);
        for (int i = 0; i < statements.count(); ++i) {
            QDomElement element = statements.at(i).toElement();
            StatementConfig statement = parseStatement(element);
            config.statements[statement.id] = statement;
        }
    }
    
    return config;
}

StatementConfig XMLMapperParser::parseStatement(const QDomElement& element)
{
    StatementConfig config;
    
    config.id = element.attribute(QStringLiteral("id"));
    config.type = parseStatementType(element.tagName());
    config.parameterType = element.attribute(QStringLiteral("parameterType"));
    config.resultType = element.attribute(QStringLiteral("resultType"));
    config.useCache = element.attribute("useCache", "false") == QStringLiteral("true");
    
    // 解析动态SQL元素
    config.dynamicElements = parseDynamicElements(element);
    
    // 提取SQL文本，包括动态元素的占位符
    config.sql = extractSqlText(element);
    
    return config;
}

StatementType XMLMapperParser::parseStatementType(const QString& tagName)
{
    if (tagName == QStringLiteral("select")) return StatementType::SELECT;
    if (tagName == QStringLiteral("insert")) return StatementType::INSERT;
    if (tagName == QStringLiteral("update")) return StatementType::UPDATE;
    if (tagName == QStringLiteral("delete")) return StatementType::DELETE;
    return StatementType::DDL;
}

QHash<QString, QString> XMLMapperParser::parseDynamicElements(const QDomElement& element)
{
    QHash<QString, QString> dynamicElements;
    
    // 解析if元素 - 只查找直接子元素
    QDomNodeList childNodes = element.childNodes();
    for (int i = 0; i < childNodes.count(); ++i) {
        QDomNode node = childNodes.at(i);
        if (node.isElement()) {
            QDomElement childElement = node.toElement();
            
            if (childElement.tagName() == QStringLiteral("if")) {
                QString test = childElement.attribute(QStringLiteral("test"));
                QString content = childElement.text().trimmed();
                if (!test.isEmpty()) {
                    dynamicElements[QStringLiteral("if_%1").arg(i)] = QStringLiteral("%1|%2").arg(test, content);
                }
            }
            else if (childElement.tagName() == QStringLiteral("foreach")) {
                QString collection = childElement.attribute(QStringLiteral("collection"));
                QString item = childElement.attribute(QStringLiteral("item"));
                QString separator = childElement.attribute("separator", ",");
                QString open = childElement.attribute("open", "");
                QString close = childElement.attribute("close", "");
                QString content = childElement.text().trimmed();
                
                if (!collection.isEmpty() && !item.isEmpty()) {
                    dynamicElements[QStringLiteral("foreach_%1").arg(i)] = 
                        QStringLiteral("%1|%2|%3|%4|%5|%6").arg(collection, item, separator, open, close, content);
                }
            }
        }
    }
    
    return dynamicElements;
}

QString XMLMapperParser::extractSqlText(const QDomElement& element)
{
    QString sql;
    
    // 遍历所有子节点，提取文本内容和动态元素占位符
    QDomNodeList childNodes = element.childNodes();
    for (int i = 0; i < childNodes.count(); ++i) {
        QDomNode node = childNodes.at(i);
        
        if (node.isText()) {
            // 文本节点，直接添加内容
            sql += node.nodeValue();
        }
        else if (node.isElement()) {
            QDomElement childElement = node.toElement();
            
            if (childElement.tagName() == QStringLiteral("if")) {
                // if元素，添加占位符
                sql += QStringLiteral(" ${if_%1} ").arg(i);
            }
            else if (childElement.tagName() == QStringLiteral("foreach")) {
                // foreach元素，添加占位符
                sql += QStringLiteral(" ${foreach_%1} ").arg(i);
            }
            else {
                // 其他元素，递归提取文本
                sql += extractSqlText(childElement);
            }
        }
    }
    
    return sql.trimmed();
}

QString XMLMapperParser::readResourceFile(const QString& resourcePath)
{
    QResource resource(resourcePath);
    
    if (!resource.isValid()) {
        throw ConfigurationException(
            QStringLiteral("Mapper resource file not found: %1. Make sure the file is added to your .qrc file")
            .arg(resourcePath)
        );
    }
    
    QByteArray data = QByteArray(reinterpret_cast<const char*>(resource.data()), resource.size());
    
    return QString::fromUtf8(data);
}

void XMLMapperParser::validateMapper(const MapperConfig& config)
{
    if (config.namespace_.isEmpty()) {
        throw ConfigurationException(QStringLiteral("Mapper namespace cannot be empty"));
    }
    
    if (config.statements.isEmpty()) {
        throw ConfigurationException(
            QStringLiteral("No statements found in mapper %1").arg(config.namespace_)
        );
    }
    
    // 检查语句ID重复
    QStringList statementIds;
    for (auto it = config.statements.begin(); it != config.statements.end(); ++it) {
        if (it.value().id.isEmpty()) {
            throw ConfigurationException(
                QStringLiteral("Statement ID cannot be empty in mapper %1").arg(config.namespace_)
            );
        }
        statementIds.append(it.value().id);
    }
}

void XMLMapperParser::checkForDuplicateStatementIds(const QList<MapperConfig>& mappers)
{
    QHash<QString, QString> statementIdToNamespace;
    
    for (const MapperConfig& mapper : mappers) {
        for (auto it = mapper.statements.begin(); it != mapper.statements.end(); ++it) {
            const QString& statementId = it.value().id;
            const QString fullId = QStringLiteral("%1.%2").arg(mapper.namespace_, statementId);
            
            if (statementIdToNamespace.contains(fullId)) {
                throw ConfigurationException(
                    QStringLiteral("Duplicate statement ID '%1' found in mappers '%2' and '%3'")
                    .arg(fullId, statementIdToNamespace[fullId], mapper.namespace_)
                );
            }
            
            statementIdToNamespace[fullId] = mapper.namespace_;
        }
    }
}

} // namespace QtMyBatisORM