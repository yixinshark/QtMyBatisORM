#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QSharedPointer>
#include <QMetaObject>
#include <QMetaMethod>
#include <QGenericArgument>
#include <QGenericReturnArgument>
#include "datamodels.h"

namespace QtMyBatisORM {

class Session;

/**
 * Mapper dynamic proxy
 * Uses Qt meta-object system to implement dynamic method calls, converting Mapper interface method calls to SQL execution
 * Mapper动态代理
 * 使用Qt元对象系统实现动态方法调用，将Mapper接口方法调用转换为SQL执行
 */
class MapperProxy : public QObject
{
    Q_OBJECT
    
public:
    explicit MapperProxy(const QString& mapperName, 
                        QSharedPointer<Session> session,
                        const MapperConfig& config,
                        QObject* parent = nullptr);
    
    // Use Qt's meta-object system to dynamically call methods
    QVariant invokeMethod(const QString& methodName, const QVariantList& args);
    
    // Support Qt meta-object system method calls
    QVariant invokeMethod(const QString& methodName, 
                         QGenericReturnArgument returnValue,
                         QGenericArgument val0 = QGenericArgument(),
                         QGenericArgument val1 = QGenericArgument(),
                         QGenericArgument val2 = QGenericArgument(),
                         QGenericArgument val3 = QGenericArgument(),
                         QGenericArgument val4 = QGenericArgument());
    
    // Convenience methods supporting different parameter types
    template<typename ReturnType>
    ReturnType invoke(const QString& methodName, const QVariantList& args = {});
    
    template<typename ReturnType, typename... Args>
    ReturnType invoke(const QString& methodName, Args&&... args);
    
    QString getMapperName() const;
    MapperConfig getConfig() const;
    
    // Check if method exists
    bool hasMethod(const QString& methodName) const;
    
    // Get method information
    QStringList getMethodNames() const;
    
private:
    QVariant executeStatement(const QString& statementId, const QVariantMap& parameters);
    QVariantMap convertArgsToParameters(const QVariantList& args, const StatementConfig& config);
    QVariantMap convertGenericArgsToParameters(const QList<QGenericArgument>& args, const StatementConfig& config);
    QString buildStatementId(const QString& methodName);
    
    // Parameter type inference and conversion
    QVariant convertArgumentToVariant(const QGenericArgument& arg);
    QString inferParameterName(int index, const StatementConfig& config);
    
    // Return value type handling
    QVariant convertReturnValue(const QVariant& result, const QString& expectedType);
    bool isListReturnType(const QString& returnType) const;
    
    QString m_mapperName;
    QSharedPointer<Session> m_session;
    MapperConfig m_config;
};

// Template method implementations
template<typename ReturnType>
ReturnType MapperProxy::invoke(const QString& methodName, const QVariantList& args)
{
    QVariant result = invokeMethod(methodName, args);
    return result.value<ReturnType>();
}

template<typename ReturnType, typename... Args>
ReturnType MapperProxy::invoke(const QString& methodName, Args&&... args)
{
    QVariantList argList;
    // Use fold expression (C++17) or manual expansion
    ((argList << QVariant::fromValue(std::forward<Args>(args))), ...);
    return invoke<ReturnType>(methodName, argList);
}

} // namespace QtMyBatisORM