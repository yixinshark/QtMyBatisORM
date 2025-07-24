#pragma once

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include "datamodels.h"

namespace QtMyBatisORM {

/**
 * Cache manager
 */
class CacheManager : public QObject
{
    Q_OBJECT
    
public:
    explicit CacheManager(const DatabaseConfig& config, QObject* parent = nullptr);
    ~CacheManager();
    
    void put(const QString& key, const QVariant& value);
    QVariant get(const QString& key);
    void remove(const QString& key);
    void clear();
    
    void invalidateByPattern(const QString& pattern);
    
    bool contains(const QString& key) const;
    int size() const;
    bool isEnabled() const;
    
    // LRU and statistics functionality
    CacheStats getStats() const;
    void resetStats();
    double getHitRate() const;
    void printStats() const;
    
    // Cache optimization functionality；缓存优化功能
    void adjustCacheSize();
    void setMaxSize(int maxSize);
    int getMaxSize() const;
    void preloadCommonQueries(const QStringList& statementIds, QSharedPointer<class Session> session);
    
private slots:
    void cleanupExpiredEntries();
    
private:
    void evictLeastRecentlyUsed();
    bool isExpired(const CacheEntry& entry) const;
    QString generateCacheKey(const QString& statementId, const QVariantMap& parameters);
    
    QHash<QString, CacheEntry> m_cache;
    QTimer* m_cleanupTimer;
    
    int m_maxSize;
    int m_expireTime;
    bool m_enabled;
    
    // Statistics information
    mutable CacheStats m_stats;
    
    // Sequence counter to ensure deterministic LRU ordering
    // 序列号计数器，用于确保LRU顺序的确定性
    qint64 m_sequenceCounter;
    
    mutable QMutex m_mutex;
};

} // namespace QtMyBatisORM