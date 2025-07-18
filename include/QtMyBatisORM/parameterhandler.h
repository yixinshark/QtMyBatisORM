#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QSqlQuery>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * 参数处理器
 */
class ParameterHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit ParameterHandler(QObject* parent = nullptr);
    
    void setParameters(QSqlQuery& query, const QVariantMap& parameters);
    QVariant convertParameter(const QVariant& value, const QString& targetType = QLatin1String(""));
    
    // 参数验证方法（公开用于测试和外部验证）
    bool isValidParameterName(const QString& name);
    
private:
    void bindByIndex(QSqlQuery& query, const QVariantMap& parameters);
    void bindByName(QSqlQuery& query, const QVariantMap& parameters);
    
    QVariant convertToSqlType(const QVariant& value);
};

} // namespace QtMyBatisORM