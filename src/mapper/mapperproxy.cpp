#include "QtMyBatisORM/mapperproxy.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaType>
#include <QDebug>
#include <QRegularExpression>

namespace QtMyBatisORM {

MapperProxy::MapperProxy(const QString& mapperName, 
                        QSharedPointer<Session> session,
                        const MapperConfig& config,
                        QObject* parent)
    : QObject(parent)
    , m_mapperName(mapperName)
    , m_session(session)
    , m_config(config)
{
    qDebug() << "Created MapperProxy for" << mapperName << "with" << config.statements.size() << "statements";
}

QVariant MapperProxy::invokeMethod(const QString& methodName, const QVariantList& args)
{
    if (!m_session) {
        throw MappingException(QStringLiteral("Session is null in MapperProxy"));
    }
    
    QString statementId = buildStatementId(methodName);
    
    if (!m_config.statements.contains(statementId)) {
        throw MappingException(
            QStringLiteral("Statement not found: %1 in mapper %2").arg(statementId, m_mapperName)
        );
    }
    
    StatementConfig statementConfig = m_config.statements[statementId];
    QVariantMap parameters = convertArgsToParameters(args, statementConfig);
    
    qDebug() << "Invoking method" << methodName << "with parameters:" << parameters;
    return executeStatement(statementId, parameters);
}

QVariant MapperProxy::invokeMethod(const QString& methodName, 
                                 QGenericReturnArgument returnValue,
                                 QGenericArgument val0,
                                 QGenericArgument val1,
                                 QGenericArgument val2,
                                 QGenericArgument val3,
                                 QGenericArgument val4)
{
    if (!m_session) {
        throw MappingException(QStringLiteral("Session is null in MapperProxy"));
    }
    
    QString statementId = buildStatementId(methodName);
    
    if (!m_config.statements.contains(statementId)) {
        throw MappingException(
            QStringLiteral("Statement not found: %1 in mapper %2").arg(statementId, m_mapperName)
        );
    }
    
    StatementConfig statementConfig = m_config.statements[statementId];
    
    // 转换QGenericArgument参数为QVariantMap
    QList<QGenericArgument> args;
    if (val0.data()) args << val0;
    if (val1.data()) args << val1;
    if (val2.data()) args << val2;
    if (val3.data()) args << val3;
    if (val4.data()) args << val4;
    
    QVariantMap parameters = convertGenericArgsToParameters(args, statementConfig);
    
    qDebug() << "Invoking method" << methodName << "with generic arguments, parameters:" << parameters;
    QVariant result = executeStatement(statementId, parameters);
    
    // 处理返回值类型转换
    if (returnValue.data()) {
        result = convertReturnValue(result, statementConfig.resultType);
    }
    
    return result;
}

QString MapperProxy::getMapperName() const
{
    return m_mapperName;
}

MapperConfig MapperProxy::getConfig() const
{
    return m_config;
}

bool MapperProxy::hasMethod(const QString& methodName) const
{
    QString statementId = QStringLiteral("%1.%2").arg(m_mapperName, methodName);
    return m_config.statements.contains(statementId);
}

QStringList MapperProxy::getMethodNames() const
{
    QStringList methodNames;
    for (auto it = m_config.statements.begin(); it != m_config.statements.end(); ++it) {
        QString statementId = it.key();
        if (statementId.startsWith(m_mapperName + ".")) {
            QString methodName = statementId.mid(m_mapperName.length() + 1);
            methodNames << methodName;
        }
    }
    return methodNames;
}

// Qt元对象系统支持的实现已注释掉，使用Q_OBJECT宏自动生成的版本
// int MapperProxy::qt_metacall(QMetaObject::Call call, int id, void** arguments)
// {
//     // 这里可以实现动态方法调用的元对象支持
//     // 暂时使用基类实现
//     return QObject::qt_metacall(call, id, arguments);
// }

QVariant MapperProxy::executeStatement(const QString& statementId, const QVariantMap& parameters)
{
    if (!m_config.statements.contains(statementId)) {
        throw MappingException(
            QStringLiteral("Statement not found: %1").arg(statementId)
        );
    }
    
    StatementConfig config = m_config.statements[statementId];
    
    switch (config.type) {
        case StatementType::SELECT:
            // 根据返回类型决定是返回单个结果还是列表
            if (config.resultType.contains(QStringLiteral("List")) || config.resultType.contains(QStringLiteral("Array"))) {
                return QVariant::fromValue(m_session->selectList(statementId, parameters));
            } else {
                return m_session->selectOne(statementId, parameters);
            }
            
        case StatementType::INSERT:
            return QVariant(m_session->insert(statementId, parameters));
            
        case StatementType::UPDATE:
            return QVariant(m_session->update(statementId, parameters));
            
        case StatementType::DELETE:
            return QVariant(m_session->remove(statementId, parameters));
            
        case StatementType::DDL:
            // DDL语句通常不返回结果
            m_session->update(statementId, parameters);
            return QVariant();
            
        default:
            throw MappingException(
                QStringLiteral("Unsupported statement type for: %1").arg(statementId)
            );
    }
}

QVariantMap MapperProxy::convertArgsToParameters(const QVariantList& args, const StatementConfig& config)
{
    Q_UNUSED(config); // 标记参数为已使用，避免警告
    QVariantMap parameters;
    
    if (args.isEmpty()) {
        return parameters;
    }
    
    // 如果只有一个参数且是QVariantMap，直接使用
    if (args.size() == 1 && args[0].typeId() == QMetaType::QVariantMap) {
        return args[0].toMap();
    }
    
    // 否则按位置创建参数映射
    for (int i = 0; i < args.size(); ++i) {
        parameters[QStringLiteral("param%1").arg(i + 1)] = args[i];
        parameters[QString::number(i)] = args[i]; // 同时支持数字索引
    }
    
    return parameters;
}

QVariantMap MapperProxy::convertGenericArgsToParameters(const QList<QGenericArgument>& args, const StatementConfig& config)
{
    QVariantMap parameters;
    
    if (args.isEmpty()) {
        return parameters;
    }
    
    for (int i = 0; i < args.size(); ++i) {
        QVariant value = convertArgumentToVariant(args[i]);
        QString paramName = inferParameterName(i, config);
        
        parameters[paramName] = value;
        parameters[QStringLiteral("param%1").arg(i + 1)] = value;
        parameters[QString::number(i)] = value;
    }
    
    return parameters;
}

QVariant MapperProxy::convertArgumentToVariant(const QGenericArgument& arg)
{
    // 根据参数类型名称转换为QVariant
    QString typeName = arg.name();
    const void* data = arg.data();
    
    if (!data) {
        return QVariant();
    }
    
    // 处理常见类型
    if (typeName == QStringLiteral("int")) {
        return QVariant(*static_cast<const int*>(data));
    } else if (typeName == QStringLiteral("QString")) {
        return QVariant(*static_cast<const QString*>(data));
    } else if (typeName == QStringLiteral("double")) {
        return QVariant(*static_cast<const double*>(data));
    } else if (typeName == QStringLiteral("bool")) {
        return QVariant(*static_cast<const bool*>(data));
    } else if (typeName == QStringLiteral("QVariant")) {
        return *static_cast<const QVariant*>(data);
    } else if (typeName == QStringLiteral("QVariantMap")) {
        return QVariant(*static_cast<const QVariantMap*>(data));
    } else {
        // 尝试使用QMetaType进行转换
        QMetaType metaType = QMetaType::fromName(typeName.toUtf8());
        if (metaType.isValid()) {
            return QVariant(metaType, data);
        }
    }
    
    qWarning() << "Unable to convert argument of type" << typeName << "to QVariant";
    return QVariant();
}

QString MapperProxy::inferParameterName(int index, const StatementConfig& config)
{
    // 尝试从SQL语句中推断参数名称
    // 这是一个简化的实现，实际应用中可能需要更复杂的解析
    QString sql = config.sql;
    
    // 查找命名参数（如 :paramName 或 #{paramName}）
    QRegularExpression namedParamRegex(R"([:#{](\w+)[}]?)");
    QRegularExpressionMatchIterator it = namedParamRegex.globalMatch(sql);
    
    int paramIndex = 0;
    while (it.hasNext() && paramIndex <= index) {
        QRegularExpressionMatch match = it.next();
        if (paramIndex == index) {
            return match.captured(1);
        }
        paramIndex++;
    }
    
    // 如果没有找到命名参数，使用默认名称
    return QStringLiteral("param%1").arg(index + 1);
}

QVariant MapperProxy::convertReturnValue(const QVariant& result, const QString& expectedType)
{
    if (expectedType.isEmpty()) {
        return result;
    }
    
    // 处理列表类型
    if (isListReturnType(expectedType)) {
        if (result.typeId() == QMetaType::QVariantList) {
            return result;
        } else {
            // 将单个结果包装为列表
            QVariantList list;
            if (result.isValid()) {
                list << result;
            }
            return QVariant::fromValue(list);
        }
    }
    
    // 处理基本类型转换
    if (expectedType == QStringLiteral("int") || expectedType == QStringLiteral("Integer")) {
        return result.toInt();
    } else if (expectedType == QStringLiteral("QString") || expectedType == QStringLiteral("String")) {
        return result.toString();
    } else if (expectedType == QStringLiteral("double") || expectedType == QStringLiteral("Double")) {
        return result.toDouble();
    } else if (expectedType == QStringLiteral("bool") || expectedType == QStringLiteral("Boolean")) {
        return result.toBool();
    }
    
    return result;
}

bool MapperProxy::isListReturnType(const QString& returnType) const
{
    return returnType.contains("List", Qt::CaseInsensitive) ||
           returnType.contains("Array", Qt::CaseInsensitive) ||
           returnType.contains("Vector", Qt::CaseInsensitive) ||
           returnType.startsWith(QStringLiteral("QList")) ||
           returnType.startsWith(QStringLiteral("QVector")) ||
           returnType.startsWith(QStringLiteral("std::vector"));
}

QString MapperProxy::buildStatementId(const QString& methodName)
{
    // 构建完整的语句ID：namespace.methodName
    return QStringLiteral("%1.%2").arg(m_mapperName, methodName);
}

} // namespace QtMyBatisORM