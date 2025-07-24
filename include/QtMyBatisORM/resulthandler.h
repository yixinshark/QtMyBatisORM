#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * Result handler
 */
class ResultHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit ResultHandler(QObject* parent = nullptr);
    
    QVariant handleSingleResult(QSqlQuery& query);
    QVariantList handleListResult(QSqlQuery& query);
    
    QVariantMap recordToMap(const QSqlQuery& query);
    QVariant convertFromSqlType(const QVariant& value, const QString& targetType = QLatin1String(""));
    
    // Utility methods (public for testing and external use)
    QStringList getColumnNames(const QSqlQuery& query);
    
private:
    QVariantMap extractRecord(const QSqlQuery& query);
    
    // Helper methods
    QVariant normalizeValue(const QVariant& value);
    QVariant parseJsonValue(const QString& jsonString);
    QVariantList parseJsonArray(const QString& jsonString);
};

} // namespace QtMyBatisORM