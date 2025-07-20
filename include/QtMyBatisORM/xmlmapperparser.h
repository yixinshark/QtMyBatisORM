#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QDomElement>
#include "datamodels.h"

namespace QtMyBatisORM {

/**
 * XML映射文件解析器
 */
class XMLMapperParser : public QObject
{
    Q_OBJECT
    
public:
    explicit XMLMapperParser(QObject* parent = nullptr);
    
    QList<MapperConfig> parseMappers(const QStringList& resourcePaths);
    MapperConfig parseMapper(const QString& resourcePath);
    
    // For testing purposes
    MapperConfig parseMapperFromDocument(const QDomDocument& doc, const QString& xmlPath);
    void checkForDuplicateStatementIds(const QList<MapperConfig>& mappers);
    
private:
    StatementConfig parseStatement(const QDomElement& element);
    StatementType parseStatementType(const QString& tagName);
    QHash<QString, QString> parseDynamicElements(const QDomElement& element);
    QString extractSqlText(const QDomElement& element);
    QString readResourceFile(const QString& resourcePath);
    void validateMapper(const MapperConfig& config);
};

} // namespace QtMyBatisORM