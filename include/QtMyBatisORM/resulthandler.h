#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * 结果处理器
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
    
    // 实用方法（公开用于测试和外部使用）
    QStringList getColumnNames(const QSqlQuery& query);
    
private:
    QVariantMap extractRecord(const QSqlQuery& query);
    
    // 辅助方法
    QVariant normalizeValue(const QVariant& value);
    QVariant parseJsonValue(const QString& jsonString);
    QVariantList parseJsonArray(const QString& jsonString);
};

} // namespace QtMyBatisORM