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
    
    // 使用Qt的元对象系统动态调用方法
    QVariant invokeMethod(const QString& methodName, const QVariantList& args);
    
    // 支持Qt元对象系统的方法调用
    QVariant invokeMethod(const QString& methodName, 
                         QGenericReturnArgument returnValue,
                         QGenericArgument val0 = QGenericArgument(),
                         QGenericArgument val1 = QGenericArgument(),
                         QGenericArgument val2 = QGenericArgument(),
                         QGenericArgument val3 = QGenericArgument(),
                         QGenericArgument val4 = QGenericArgument());
    
    // 支持不同参数类型的便捷方法
    template<typename ReturnType>
    ReturnType invoke(const QString& methodName, const QVariantList& args = {});
    
    template<typename ReturnType, typename... Args>
    ReturnType invoke(const QString& methodName, Args&&... args);
    
    QString getMapperName() const;
    MapperConfig getConfig() const;
    
    // 检查方法是否存在
    bool hasMethod(const QString& methodName) const;
    
    // 获取方法信息
    QStringList getMethodNames() const;
    
protected:
    // Qt元对象系统支持 - 注释掉，使用Q_OBJECT宏自动生成的版本
    // int qt_metacall(QMetaObject::Call call, int id, void** arguments) override;
    
private:
    QVariant executeStatement(const QString& statementId, const QVariantMap& parameters);
    QVariantMap convertArgsToParameters(const QVariantList& args, const StatementConfig& config);
    QVariantMap convertGenericArgsToParameters(const QList<QGenericArgument>& args, const StatementConfig& config);
    QString buildStatementId(const QString& methodName);
    
    // 参数类型推断和转换
    QVariant convertArgumentToVariant(const QGenericArgument& arg);
    QString inferParameterName(int index, const StatementConfig& config);
    
    // 返回值类型处理
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