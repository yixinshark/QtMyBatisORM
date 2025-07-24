#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QSqlQuery>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * Parameter handler
 */
class ParameterHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit ParameterHandler(QObject* parent = nullptr);
    
    void setParameters(QSqlQuery& query, const QVariantMap& parameters);
    QVariant convertParameter(const QVariant& value, const QString& targetType = QLatin1String(""));
    
    // Parameter validation methods (public for testing and external validation)
    bool isValidParameterName(const QString& name);
    
private:
    void bindByIndex(QSqlQuery& query, const QVariantMap& parameters);
    void bindByName(QSqlQuery& query, const QVariantMap& parameters);
    
    QVariant convertToSqlType(const QVariant& value);
};

} // namespace QtMyBatisORM