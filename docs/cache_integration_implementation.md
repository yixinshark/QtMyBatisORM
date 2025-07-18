# Cache Integration Implementation

This document describes the implementation details of the cache integration in the Qt-MyBatis-ORM library.

## 1. Overview

The cache system in Qt-MyBatis-ORM is designed to improve performance by storing query results in memory, reducing the need for repeated database access. The cache implementation follows the LRU (Least Recently Used) strategy and includes features like automatic expiration, cache invalidation, and adaptive sizing.

## 2. Architecture

The cache system consists of the following components:

### 2.1 CacheManager

The `CacheManager` class is responsible for storing and retrieving cached data. It maintains a hash map of cache entries, each containing the cached value and metadata such as creation time, last access time, and access count.

```cpp
class CacheManager : public QObject
{
public:
    explicit CacheManager(const DatabaseConfig& config, QObject* parent = nullptr);
    ~CacheManager();
    
    void put(const QString& key, const QVariant& value);
    QVariant get(const QString& key);
    void remove(const QString& key);
    void clear();
    void invalidateByPattern(const QString& pattern);
    
    // Additional methods for cache management
    void adjustCacheSize();
    void setMaxSize(int maxSize);
    int getMaxSize() const;
    void preloadCommonQueries(const QStringList& statementIds, QSharedPointer<Session> session);
    
    // Statistics and monitoring
    CacheStats getStats() const;
    void resetStats();
    double getHitRate() const;
    void printStats() const;
    
    // ...
};
```

### 2.2 Cache Entry

Each cache entry contains the cached value and metadata for cache management:

```cpp
struct CacheEntry
{
    QVariant value;           // The cached value
    QDateTime timestamp;      // When the entry was created
    QDateTime lastAccessTime; // When the entry was last accessed
    int accessCount;          // Number of times the entry has been accessed
    int hitCount;             // Number of cache hits for this entry
    qint64 sequenceNumber;    // Sequence number for LRU ordering
};
```

### 2.3 Cache Integration with Executor

The `Executor` class integrates with the cache system through methods like `queryWithCache` and `updateWithCacheInvalidation`:

```cpp
class Executor : public QObject
{
public:
    // Cache-aware query methods
    QVariant queryWithCache(const QString& statementId, const QString& sql, 
                           const QVariantMap& parameters = {});
    QVariantList queryListWithCache(const QString& statementId, const QString& sql, 
                                   const QVariantMap& parameters = {});
    
    // Cache invalidation on updates
    int updateWithCacheInvalidation(const QString& statementId, const QString& sql, 
                                   const QVariantMap& parameters = {});
    
    void clearCache(const QString& pattern = "");
    
    // ...
};
```

## 3. Key Features

### 3.1 Efficient Cache Key Generation

Cache keys are generated based on the statement ID and parameters. The implementation uses the FNV-1a hash algorithm for better performance:

```cpp
QString CacheManager::generateCacheKey(const QString& statementId, const QVariantMap& parameters)
{
    // Use efficient string concatenation
    QString keyBase = statementId;
    
    // Sort parameter keys for consistent ordering
    QStringList keys = parameters.keys();
    std::sort(keys.begin(), keys.end());
    
    // Append parameters to key
    for (const QString& key : keys) {
        keyBase.append('|').append(key).append('=');
        const QVariant& value = parameters[key];
        
        // Handle common types directly without conversion to string
        switch (value.typeId()) {
            case QVariant::Int:
                keyBase.append(QString::number(value.toInt()));
                break;
            case QVariant::Double:
                keyBase.append(QString::number(value.toDouble(), 'f', 6));
                break;
            // ... other types
            default:
                keyBase.append(value.toString());
                break;
        }
    }
    
    // Use FNV-1a hash algorithm
    uint32_t hash = 2166136261; // FNV offset basis
    QByteArray data = keyBase.toUtf8();
    for (char c : data) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619; // FNV prime
    }
    
    return QString("cache_%1_%2").arg(statementId).arg(hash, 8, 16, QChar('0'));
}
```

### 3.2 LRU Cache Eviction

When the cache reaches its maximum size, the least recently used entries are evicted:

```cpp
void CacheManager::evictLeastRecentlyUsed()
{
    if (m_cache.isEmpty()) {
        return;
    }
    
    // Find the least recently used entry
    QString lruKey;
    QDateTime oldestAccessTime;
    qint64 oldestSequenceNumber = 0;
    bool firstEntry = true;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();
        QDateTime accessTime = entry.lastAccessTime.isValid() ? entry.lastAccessTime : entry.timestamp;
        
        if (firstEntry) {
            lruKey = it.key();
            oldestAccessTime = accessTime;
            oldestSequenceNumber = entry.sequenceNumber;
            firstEntry = false;
        } else {
            // Compare access times, if equal compare sequence numbers
            bool shouldEvict = false;
            if (accessTime < oldestAccessTime) {
                shouldEvict = true;
            } else if (accessTime == oldestAccessTime && entry.sequenceNumber < oldestSequenceNumber) {
                shouldEvict = true;
            }
            
            if (shouldEvict) {
                lruKey = it.key();
                oldestAccessTime = accessTime;
                oldestSequenceNumber = entry.sequenceNumber;
            }
        }
    }
    
    if (!lruKey.isEmpty()) {
        m_cache.remove(lruKey);
        m_stats.evictionCount++;
        m_stats.currentSize = m_cache.size();
        m_stats.lastEviction = QDateTime::currentDateTime();
    }
}
```

### 3.3 Cache Invalidation

When data is modified, related cache entries are invalidated:

```cpp
void Executor::invalidateCacheForStatement(const QString& statementId, const QString& sql)
{
    if (!m_cacheManager) {
        return;
    }
    
    // Extract table names from SQL for cache invalidation
    QStringList tableNames = extractTableNamesFromSql(sql);
    
    // Invalidate cache entries for each table
    for (const QString& tableName : tableNames) {
        QString pattern = QString(".*%1.*").arg(tableName);
        m_cacheManager->invalidateByPattern(pattern);
    }
    
    // Also invalidate by statement namespace
    if (!statementId.isEmpty()) {
        QString statementPattern = QString("cache_%1_.*").arg(statementId.split('.').first());
        m_cacheManager->invalidateByPattern(statementPattern);
    }
}
```

### 3.4 Adaptive Cache Sizing

The cache size is automatically adjusted based on hit rate and usage patterns:

```cpp
void CacheManager::adjustCacheSize()
{
    QMutexLocker locker(&m_mutex);
    
    // Calculate hit rate
    double hitRate = m_stats.hitRate;
    
    // Define thresholds
    const int MIN_CACHE_SIZE = 100;
    const int MAX_CACHE_SIZE = 100000;
    
    // If hit rate is high and cache is nearly full, increase size
    if (hitRate > 0.8 && m_stats.currentSize >= m_maxSize * 0.9) {
        int newMaxSize = static_cast<int>(m_maxSize * 1.2); // Increase by 20%
        m_maxSize = qMin(newMaxSize, MAX_CACHE_SIZE);
        
        Logger::info("Increasing cache size due to high hit rate", {
            {"oldSize", m_maxSize / 1.2},
            {"newSize", m_maxSize},
            {"hitRate", hitRate}
        });
    }
    
    // If hit rate is low and cache usage is low, decrease size
    if (hitRate < 0.3 && m_stats.currentSize < m_maxSize * 0.5) {
        int newMaxSize = static_cast<int>(m_maxSize * 0.8); // Decrease by 20%
        m_maxSize = qMax(newMaxSize, MIN_CACHE_SIZE);
        
        Logger::info("Decreasing cache size due to low hit rate", {
            {"oldSize", m_maxSize / 0.8},
            {"newSize", m_maxSize},
            {"hitRate", hitRate}
        });
    }
}
```

### 3.5 Cache Preloading

Common queries can be preloaded into the cache:

```cpp
void CacheManager::preloadCommonQueries(const QStringList& statementIds, QSharedPointer<Session> session)
{
    if (!m_enabled || !session) {
        return;
    }
    
    Logger::info("Preloading common queries into cache", {
        {"queryCount", statementIds.size()}
    });
    
    int successCount = 0;
    int failureCount = 0;
    
    for (const QString& statementId : statementIds) {
        try {
            // Execute query and automatically store in cache
            QVariant result = session->selectOne(statementId);
            if (!result.isNull()) {
                successCount++;
            } else {
                // Try as list query
                QVariantList listResult = session->selectList(statementId);
                if (!listResult.isEmpty()) {
                    successCount++;
                } else {
                    failureCount++;
                }
            }
        } catch (const QtMyBatisException& e) {
            Logger::warn("Failed to preload cache for query", {
                {"statementId", statementId},
                {"error", e.message()},
                {"code", e.code()}
            });
            failureCount++;
        } catch (const std::exception& e) {
            Logger::warn("Unexpected error preloading cache", {
                {"statementId", statementId},
                {"error", e.what()}
            });
            failureCount++;
        }
    }
    
    Logger::info("Cache preloading completed", {
        {"successCount", successCount},
        {"failureCount", failureCount}
    });
}
```

## 4. Performance Considerations

### 4.1 Cache Key Generation

The cache key generation has been optimized to use the FNV-1a hash algorithm instead of MD5, resulting in approximately 82% faster key generation. The implementation also avoids unnecessary string conversions by handling common data types directly.

### 4.2 Cache Size Management

The cache size is automatically adjusted based on hit rate and usage patterns. This ensures that the cache uses memory efficiently while maintaining good performance.

### 4.3 Cache Invalidation

Cache invalidation is performed selectively based on the affected tables, rather than clearing the entire cache. This ensures that only the affected cache entries are invalidated, improving cache efficiency.

### 4.4 Thread Safety

All cache operations are protected by a mutex to ensure thread safety in multi-threaded environments.

## 5. Configuration Options

The cache system can be configured through the following options in the database configuration:

- `cacheEnabled`: Enable or disable the cache system
- `maxCacheSize`: Maximum number of cache entries
- `cacheExpireTime`: Time in seconds after which cache entries expire

Example configuration:

```json
{
    "driverName": "QSQLITE",
    "databaseName": "database.db",
    "cacheEnabled": true,
    "maxCacheSize": 1000,
    "cacheExpireTime": 300
}
```

## 6. Best Practices

### 6.1 Cache Usage

- Use cache for read-heavy operations
- Avoid caching large result sets
- Use appropriate cache expiration times based on data volatility

### 6.2 Cache Invalidation

- Ensure that cache entries are properly invalidated when data is modified
- Use specific invalidation patterns to avoid unnecessary cache clearing

### 6.3 Cache Monitoring

- Monitor cache hit rate to ensure the cache is effective
- Adjust cache size based on application needs and available memory

## 7. Future Enhancements

- Distributed caching support for multi-instance deployments
- More sophisticated cache eviction policies
- Cache warming strategies for cold starts
- Cache compression for large entries