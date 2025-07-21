#pragma once

#include <QString>
#include <QVariantMap>
#include <QHash>
#include <QDateTime>

namespace QtMyBatisORM {

/**
 * 数据库配置结构
 */
struct DatabaseConfig
{
    // 基本数据库配置
    QString driverName;     // QMYSQL, QSQLITE (JSON: type)
    QString hostName;       // JSON: host
    int port = 3306;
    QString databaseName;   // JSON: database_name
    QString userName;       // JSON: username
    QString password;
    
    // 调试配置
    bool debug = false;     // 是否开启SQL调试日志
    
    // 连接池配置
    int maxConnections = 10;        // JSON: max_connection_count
    int minConnections = 2;         // 固定值
    int maxIdleTime = 300;          // 固定值（秒）
    int maxWaitTime = 5000;         // JSON: max_wait_time（毫秒）
    
    // 缓存配置（固定值，简化配置）
    bool cacheEnabled = true;
    int maxCacheSize = 1000;
    int cacheExpireTime = 600;      // 秒
    
    // SQL文件列表
    QStringList sqlFiles;           // JSON: sql_files
};

/**
 * SQL语句类型枚举
 */
enum class StatementType
{
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    DDL  // CREATE, ALTER, DROP等
};

/**
 * SQL语句配置
 */
struct StatementConfig
{
    QString id;
    QString sql;
    StatementType type;
    QString parameterType;
    QString resultType;
    bool useCache = false;
    QHash<QString, QString> dynamicElements;  // if, foreach等动态元素
};

/**
 * Mapper配置
 */
struct MapperConfig
{
    QString namespace_;
    QString xmlPath;
    QHash<QString, StatementConfig> statements;
    QHash<QString, QString> resultMaps;  // 结果映射配置
};

/**
 * 缓存条目
 */
struct CacheEntry
{
    QVariant value;
    QDateTime timestamp;
    QDateTime lastAccessTime;  // 最后访问时间，用于LRU策略
    int accessCount = 0;
    int hitCount = 0;  // 命中次数统计
    qint64 sequenceNumber = 0;  // 序列号，用于确保LRU顺序的确定性
};

/**
 * 连接池统计信息
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
    double averageConnectionAge = 0.0;  // 平均连接存活时间（秒）
    int peakUsedConnections = 0;
};

/**
 * 连接健康状态
 */
enum class ConnectionHealth
{
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    UNKNOWN
};

/**
 * 连接池健康报告
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
 * 缓存统计信息
 */
struct CacheStats
{
    int totalRequests = 0;      // 总请求数
    int hitCount = 0;           // 命中次数
    int missCount = 0;          // 未命中次数
    int evictionCount = 0;      // 驱逐次数
    int expiredCount = 0;       // 过期清理次数
    double hitRate = 0.0;       // 命中率
    int currentSize = 0;        // 当前缓存大小
    int maxSize = 0;            // 最大缓存大小
    QDateTime lastAccess;       // 最后访问时间
    QDateTime lastEviction;     // 最后驱逐时间
    QDateTime lastExpiration;   // 最后过期清理时间
    
    // 计算命中率
    void updateHitRate() {
        if (totalRequests > 0) {
            hitRate = static_cast<double>(hitCount) / totalRequests;
        }
    }
};

} // namespace QtMyBatisORM