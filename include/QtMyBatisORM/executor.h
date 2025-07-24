#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QSharedPointer>
#include <QMutex>

namespace QtMyBatisORM {

class StatementHandler;
class ParameterHandler;
class ResultHandler;
class CacheManager;

/**
 * SQL executor
 */
class Executor : public QObject
{
    Q_OBJECT
    
public:
    explicit Executor(QSharedPointer<QSqlDatabase> connection, 
                     QSharedPointer<CacheManager> cacheManager,
                     QObject* parent = nullptr);
    
    QVariant query(const QString& sql, const QVariantMap& parameters = {});
    QVariantList queryList(const QString& sql, const QVariantMap& parameters = {});
    int update(const QString& sql, const QVariantMap& parameters = {});
    
    QVariant queryWithCache(const QString& statementId, const QString& sql, 
                           const QVariantMap& parameters = {});
    QVariantList queryListWithCache(const QString& statementId, const QString& sql, 
                                   const QVariantMap& parameters = {});
    
    int updateWithCacheInvalidation(const QString& statementId, const QString& sql, 
                                   const QVariantMap& parameters = {});
    
    void clearCache(const QString& pattern = QLatin1String(""));
    
    void setDebugMode(bool enabled);
    [[nodiscard]] bool isDebugMode() const;
    
    // For testing purposes
    QString generateCacheKey(const QString& statementId, const QVariantMap& parameters);
    
private:
    QVariant queryInternal(const QString& sql, const QVariantMap& parameters, 
                          const QString& statementId, bool useCache);
    QVariantList queryListInternal(const QString& sql, const QVariantMap& parameters, 
                                  const QString& statementId, bool useCache);
    int updateInternal(const QString& sql, const QVariantMap& parameters, 
                      const QString& statementId, bool invalidateCache);
    
    void invalidateCacheForStatement(const QString& statementId, const QString& sql);
    QStringList extractTableNamesFromSql(const QString& sql);
    
    // Get processed SQL statement, preferentially from cache
    QString getProcessedSql(const QString& sql, const QVariantMap& parameters);
    
    // Use object pool or fallback strategy to bind parameters
    void withParameterHandler(QSqlQuery &query, const QVariantMap &parameters);
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<StatementHandler> m_statementHandler;
    QSharedPointer<ParameterHandler> m_parameterHandler;
    QSharedPointer<ResultHandler> m_resultHandler;
    QSharedPointer<CacheManager> m_cacheManager;
    QHash<QString, QString> m_processedSqlCache; // SQL processing cache
    QMutex m_sqlCacheMutex; // Mutex to protect SQL cache
    
    // Debug related
    bool m_debugMode;
    void logDebugInfo(const QString& operation, const QString& sql, 
                     const QVariantMap& parameters, qint64 elapsedMs, 
                     const QVariant& result = QVariant()) const;
    void logSqlExecutionFlow(const QString& operation, const QString& originalSql, 
                            const QVariantMap& parameters, const QString& finalSql,
                            qint64 elapsedMs, const QVariant& result = QVariant()) const;
};

} // namespace QtMyBatisORM