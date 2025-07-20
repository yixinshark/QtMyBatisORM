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
 * SQL执行器
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
    
    // 调试功能
    void setDebugMode(bool enabled);
    bool isDebugMode() const;
    
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
    
    // 获取处理后的SQL语句，优先从缓存中获取
    QString getProcessedSql(const QString& sql, const QVariantMap& parameters);
    
    // 使用对象池或回退策略绑定参数
    void withParameterHandler(QSqlQuery &query, const QVariantMap &parameters);
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<StatementHandler> m_statementHandler;
    QSharedPointer<ParameterHandler> m_parameterHandler;
    QSharedPointer<ResultHandler> m_resultHandler;
    QSharedPointer<CacheManager> m_cacheManager;
    QHash<QString, QString> m_processedSqlCache; // SQL处理缓存
    QMutex m_sqlCacheMutex; // 保护SQL缓存的互斥锁
    
    // 调试相关
    bool m_debugMode;
    void logDebugInfo(const QString& operation, const QString& sql, 
                     const QVariantMap& parameters, qint64 elapsedMs, 
                     const QVariant& result = QVariant()) const;
};

} // namespace QtMyBatisORM