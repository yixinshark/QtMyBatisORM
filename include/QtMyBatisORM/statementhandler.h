#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QString>
#include <QVariantMap>

namespace QtMyBatisORM {

class DynamicSqlProcessor;

/**
 * SQL statement handler
 */
class StatementHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit StatementHandler(QObject* parent = nullptr);
    
    QSqlQuery prepare(const QString& sql, QSqlDatabase& db);
    void setParameters(QSqlQuery& query, const QVariantMap& parameters);
    
    QString processSql(const QString& sql, const QVariantMap& parameters);
    
private:
    void bindParameter(QSqlQuery& query, const QString& placeholder, const QVariant& value);
    QStringList extractPlaceholders(const QString& sql);
    
    DynamicSqlProcessor* m_dynamicProcessor;
};

} // namespace QtMyBatisORM