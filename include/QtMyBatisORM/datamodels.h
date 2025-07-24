#pragma once

#include <QString>
#include <QVariantMap>
#include <QHash>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * Database configuration structure
 */
struct DatabaseConfig
{
    // Basic database configuration
    QString driverName;     // QMYSQL, QSQLITE (JSON: type)
    QString hostName;       // JSON: host
    int port = 3306;
    QString databaseName;   // JSON: database_name
    QString userName;       // JSON: username
    QString password;
    
    // Debug configuration
    bool debug = false;     // Whether to enable SQL debug logging
    
    // Connection pool configuration
    int maxConnections = 10;        // JSON: max_connection_count
    int minConnections = 2;         // Fixed value
    int maxIdleTime = 300;          // Fixed value (seconds)
    int maxWaitTime = 5000;         // JSON: max_wait_time (milliseconds)
    
    // Cache configuration
    bool cacheEnabled = true;
    int maxCacheSize = 1000;
    int cacheExpireTime = 600;      // seconds
    
    // SQL file list
    QStringList sqlFiles;           // JSON: sql_files
};

/**
 * SQL statement type enumeration
 */
enum class StatementType
{
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    DDL  // CREATE, ALTER, DROP, etc.
};

/**
 * SQL statement configuration
 */
struct StatementConfig
{
    QString id;
    QString sql;
    StatementType type;
    QString parameterType;
    QString resultType;
    bool useCache = false;
    QHash<QString, QString> dynamicElements;  // Dynamic elements like if, foreach, etc.
};

/**
 * Mapper configuration
 */
struct MapperConfig
{
    QString namespace_;
    QString xmlPath;
    QHash<QString, StatementConfig> statements;
    QHash<QString, QString> resultMaps;  // Result mapping configuration
};

/**
 * Cache entry
 */
struct CacheEntry
{
    QVariant value;
    QDateTime timestamp;
    QDateTime lastAccessTime;  // Last access time, used for LRU strategy
    int accessCount = 0;
    int hitCount = 0;  // Hit count statistics
    qint64 sequenceNumber = 0;  // Sequence number to ensure deterministic LRU ordering
};

/**
 * Connection pool statistics
 */
struct ConnectionPoolStats
{
    int totalConnections = 0;
    int availableConnections = 0;
    int usedConnections = 0;
    int maxConnectionsReached = 0;
    int totalConnectionsCreated = 0;
    int totalConnectionsDestroyed = 0;
    int connectionFailures = 0;
    int connectionTimeouts = 0;
    QDateTime lastConnectionCreated;
    QDateTime lastConnectionDestroyed;
    double averageConnectionAge = 0.0;  // Average connection lifetime (seconds)
    int peakUsedConnections = 0;
};

/**
 * Connection health status
 */
enum class ConnectionHealth
{
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    UNKNOWN
};

/**
 * Connection pool health report
 */
struct ConnectionPoolHealth
{
    ConnectionHealth overallHealth = ConnectionHealth::UNKNOWN;
    QString healthMessage;
    int healthyConnections = 0;
    int unhealthyConnections = 0;
    int failedHealthChecks = 0;
    QDateTime lastHealthCheck;
    QStringList warnings;
    QStringList errors;
};

/**
 * Cache statistics
 */
struct CacheStats
{
    int totalRequests = 0;      // Total request count;总请求数
    int hitCount = 0;           // Hit count;命中次数
    int missCount = 0;          // Miss count;未命中次数
    int evictionCount = 0;      // Eviction count;驱逐次数
    int expiredCount = 0;       // Expired cleanup count;过期清理次数
    double hitRate = 0.0;       // Hit rate;命中率
    int currentSize = 0;        // Current cache size;当前缓存大小
    int maxSize = 0;            // Maximum cache size
    QDateTime lastAccess;       // Last access time;最后访问时间
    QDateTime lastEviction;     // Last eviction time;最后驱逐时间
    QDateTime lastExpiration;   // Last expiration cleanup time;最后过期清理时间
    
    // Calculate hit rate
    void updateHitRate() {
        if (totalRequests > 0) {
            hitRate = static_cast<double>(hitCount) / totalRequests;
        }
    }
};

} // namespace QtMyBatisORM