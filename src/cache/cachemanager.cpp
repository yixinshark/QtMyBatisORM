#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/logger.h"
#include "QtMyBatisORM/session.h"
#include <QMutexLocker>
#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QDebug>

namespace QtMyBatisORM {

CacheManager::CacheManager(const DatabaseConfig& config, QObject* parent)
    : QObject(parent)
    , m_maxSize(config.maxCacheSize)
    , m_expireTime(config.cacheExpireTime)
    , m_enabled(config.cacheEnabled)
    , m_sequenceCounter(0)
{
    // 初始化统计信息
    m_stats.maxSize = m_maxSize;
    
    // 设置清理定时器
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &CacheManager::cleanupExpiredEntries);
    
    if (m_enabled) {
        m_cleanupTimer->start(30000); // 每30秒清理一次过期条目
    }
}

CacheManager::~CacheManager()
{
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
}

void CacheManager::put(const QString& key, const QVariant& value)
{
    if (!m_enabled) {
        return;
    }
    
    if (key.isEmpty()) {
        CacheException ex("Cache key cannot be empty", "CACHE_EMPTY_KEY");
        ex.setContext(QStringLiteral("operation"), "put");
        ex.setContext(QStringLiteral("valueType"), value.typeName());
        throw ex;
    }
    
    try {
        QMutexLocker locker(&m_mutex);
        
        QDateTime now = QDateTime::currentDateTime();
        
        // 如果key已存在，直接更新
        if (m_cache.contains(key)) {
            CacheEntry& entry = m_cache[key];
            entry.value = value;
            entry.timestamp = now;
            entry.lastAccessTime = now;
            entry.accessCount++;
            return;
        }
        
        // 检查是否需要清理空间（只有在添加新key时）
        if (m_cache.size() >= m_maxSize) {
            try {
                evictLeastRecentlyUsed();
            } catch (const std::exception& e) {
                CacheException ex(
                    QStringLiteral("Failed to evict cache entries: %1").arg(QString::fromUtf8(e.what())),
                    "CACHE_EVICTION_ERROR"
                );
                ex.setContext(QStringLiteral("operation"), "put");
                ex.setContext(QStringLiteral("key"), key);
                ex.setContext(QStringLiteral("cacheSize"), m_cache.size());
                ex.setContext(QStringLiteral("maxSize"), m_maxSize);
                throw ex;
            }
        }
        
        CacheEntry entry;
        entry.value = value;
        entry.timestamp = now;
        entry.lastAccessTime = now;  // 初始时lastAccessTime等于timestamp
        entry.accessCount = 1;
        entry.hitCount = 0;
        entry.sequenceNumber = ++m_sequenceCounter;  // 分配序列号确保顺序
        
        m_cache[key] = entry;
        m_stats.currentSize = m_cache.size();
        
    } catch (const CacheException& e) {
        CacheException ex(e);
        ex.setContext(QStringLiteral("operation"), "put");
        ex.setContext(QStringLiteral("key"), key);
        throw ex;
    } catch (const std::exception& e) {
        CacheException ex(
            QStringLiteral("Unexpected error putting cache entry: %1").arg(QString::fromUtf8(e.what())),
            "CACHE_UNEXPECTED_ERROR"
        );
        ex.setContext(QStringLiteral("operation"), "put");
        ex.setContext(QStringLiteral("key"), key);
        ex.setContext(QStringLiteral("stdError"), e.what());
        throw ex;
    }
}

QVariant CacheManager::get(const QString& key)
{
    if (!m_enabled) {
        return QVariant();
    }
    
    if (key.isEmpty()) {
        CacheException ex("Cache key cannot be empty", "CACHE_EMPTY_KEY");
        ex.setContext(QStringLiteral("operation"), "get");
        throw ex;
    }
    
    try {
        QMutexLocker locker(&m_mutex);
        
        // 更新统计信息
        m_stats.totalRequests++;
        m_stats.lastAccess = QDateTime::currentDateTime();
        
        if (!m_cache.contains(key)) {
            m_stats.missCount++;
            m_stats.updateHitRate();
            return QVariant();
        }
        
        CacheEntry& entry = m_cache[key];
        
        // 检查是否过期
        if (isExpired(entry)) {
            m_cache.remove(key);
            m_stats.missCount++;
            m_stats.expiredCount++;
            m_stats.currentSize = m_cache.size();
            m_stats.lastExpiration = QDateTime::currentDateTime();
            m_stats.updateHitRate();
            return QVariant();
        }
        
        // 更新访问统计 - 命中
        m_stats.hitCount++;
        entry.accessCount++;
        entry.hitCount++;
        entry.lastAccessTime = QDateTime::currentDateTime();
        m_stats.updateHitRate();
        
        return entry.value;
        
    } catch (const CacheException& e) {
        CacheException ex(e);
        ex.setContext(QStringLiteral("operation"), "get");
        ex.setContext(QStringLiteral("key"), key);
        throw ex;
    } catch (const std::exception& e) {
        CacheException ex(
            QStringLiteral("Unexpected error getting cache entry: %1").arg(QString::fromUtf8(e.what())),
            "CACHE_UNEXPECTED_ERROR"
        );
        ex.setContext(QStringLiteral("operation"), "get");
        ex.setContext(QStringLiteral("key"), key);
        ex.setContext(QStringLiteral("stdError"), e.what());
        throw ex;
    }
}

void CacheManager::remove(const QString& key)
{
    if (!m_enabled) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    m_cache.remove(key);
    m_stats.currentSize = m_cache.size();
}

void CacheManager::clear()
{
    if (!m_enabled) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
    m_stats.currentSize = 0;
}

void CacheManager::invalidateByPattern(const QString& pattern)
{
    if (!m_enabled) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QRegularExpression regex(pattern);
    QStringList keysToRemove;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (regex.match(it.key()).hasMatch()) {
            keysToRemove.append(it.key());
        }
    }
    
    for (const QString& key : keysToRemove) {
        m_cache.remove(key);
    }
}

bool CacheManager::contains(const QString& key) const
{
    if (!m_enabled) {
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(key);
}

int CacheManager::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.size();
}

bool CacheManager::isEnabled() const
{
    return m_enabled;
}

void CacheManager::cleanupExpiredEntries()
{
    if (!m_enabled) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QStringList expiredKeys;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (isExpired(it.value())) {
            expiredKeys.append(it.key());
        }
    }
    
    if (!expiredKeys.isEmpty()) {
        for (const QString& key : expiredKeys) {
            m_cache.remove(key);
        }
        m_stats.expiredCount += expiredKeys.size();
        m_stats.currentSize = m_cache.size();
        m_stats.lastExpiration = QDateTime::currentDateTime();
    }
}

void CacheManager::evictLeastRecentlyUsed()
{
    if (m_cache.isEmpty()) {
        return;
    }
    
    // 找到最久未访问的条目 - 使用lastAccessTime和sequenceNumber进行LRU策略
    QString lruKey;
    QDateTime oldestAccessTime;
    qint64 oldestSequenceNumber = 0;
    bool firstEntry = true;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();
        // 使用lastAccessTime而不是timestamp来实现真正的LRU
        QDateTime accessTime = entry.lastAccessTime.isValid() ? entry.lastAccessTime : entry.timestamp;
        
        if (firstEntry) {
            lruKey = it.key();
            oldestAccessTime = accessTime;
            oldestSequenceNumber = entry.sequenceNumber;
            firstEntry = false;
        } else {
            // 比较访问时间，如果时间相同则比较序列号（较小的序列号表示更早创建）
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

bool CacheManager::isExpired(const CacheEntry& entry) const
{
    if (m_expireTime <= 0) {
        return false; // 永不过期
    }
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsElapsed = entry.timestamp.secsTo(now);
    return secondsElapsed >= m_expireTime;
}

QString CacheManager::generateCacheKey(const QString& statementId, const QVariantMap& parameters)
{
    // 使用更高效的字符串拼接方法
    QString keyBase = statementId;
    
    // 对参数键进行排序以确保一致的顺序
    QStringList keys = parameters.keys();
    std::sort(keys.begin(), keys.end());
    
    // 将参数附加到键
    for (const QString& key : keys) {
        keyBase.append(QLatin1Char('|')).append(key).append(QLatin1Char('='));
        const QVariant& value = parameters[key];
        
        // 直接处理常见类型，避免转换为字符串
        switch (value.typeId()) {
            case QMetaType::Int:
                keyBase.append(QString::number(value.toInt()));
                break;
            case QMetaType::LongLong:
                keyBase.append(QString::number(value.toLongLong()));
                break;
            case QMetaType::Double:
                keyBase.append(QString::number(value.toDouble(), 'f', 6));
                break;
            case QMetaType::Bool:
                keyBase.append(value.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                break;
            case QMetaType::QDateTime:
                keyBase.append(value.toDateTime().toString(Qt::ISODate));
                break;
            case QMetaType::QDate:
                keyBase.append(value.toDate().toString(Qt::ISODate));
                break;
            case QMetaType::QTime:
                keyBase.append(value.toTime().toString(Qt::ISODate));
                break;
            default:
                keyBase.append(value.toString());
                break;
        }
    }
    
    // 使用更快的FNV-1a哈希算法代替MD5
    uint32_t hash = 2166136261; // FNV偏移基数
    QByteArray data = keyBase.toUtf8();
    for (char c : data) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619; // FNV素数
    }
    
    return QStringLiteral("cache_%1_%2").arg(statementId).arg(hash, 8, 16, QLatin1Char('0'));
}

CacheStats CacheManager::getStats() const
{
    QMutexLocker locker(&m_mutex);
    CacheStats stats = m_stats;
    stats.currentSize = m_cache.size();
    return stats;
}

void CacheManager::resetStats()
{
    QMutexLocker locker(&m_mutex);
    m_stats = CacheStats();
    m_stats.maxSize = m_maxSize;
    m_stats.currentSize = m_cache.size();
}

double CacheManager::getHitRate() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats.hitRate;
}

void CacheManager::printStats() const
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "=== Cache Statistics ===";
    qDebug() << "Total Requests:" << m_stats.totalRequests;
    qDebug() << "Hit Count:" << m_stats.hitCount;
    qDebug() << "Miss Count:" << m_stats.missCount;
    qDebug() << "Hit Rate:" << QString::number(m_stats.hitRate * 100, 'f', 2) << "%";
    qDebug() << "Eviction Count:" << m_stats.evictionCount;
    qDebug() << "Expired Count:" << m_stats.expiredCount;
    qDebug() << "Current Size:" << m_stats.currentSize;
    qDebug() << "Max Size:" << m_stats.maxSize;
    qDebug() << "Last Access:" << m_stats.lastAccess.toString();
    qDebug() << "Last Eviction:" << m_stats.lastEviction.toString();
    qDebug() << "Last Expiration:" << m_stats.lastExpiration.toString();
    qDebug() << "========================";
}

// 自适应缓存大小调整
void CacheManager::adjustCacheSize()
{
    QMutexLocker locker(&m_mutex);
    
    // 计算最近一段时间的命中率
    double hitRate = m_stats.hitRate;
    
    // 定义缓存大小调整的阈值
    const int MIN_CACHE_SIZE = 100;
    const int MAX_CACHE_SIZE = 100000;
    
    // 如果命中率高且缓存接近满，考虑增加缓存大小
    if (hitRate > 0.8 && m_stats.currentSize >= m_maxSize * 0.9) {
        int newMaxSize = static_cast<int>(m_maxSize * 1.2); // 增加20%
        m_maxSize = qMin(newMaxSize, MAX_CACHE_SIZE);
        
        Logger::info(QStringLiteral("Increasing cache size due to high hit rate"), {
            {"oldSize", m_maxSize / 1.2},
            {"newSize", m_maxSize},
            {"hitRate", hitRate}
        });
    }
    
    // 如果命中率低且缓存使用率低，考虑减少缓存大小
    if (hitRate < 0.3 && m_stats.currentSize < m_maxSize * 0.5) {
        int newMaxSize = static_cast<int>(m_maxSize * 0.8); // 减少20%
        m_maxSize = qMax(newMaxSize, MIN_CACHE_SIZE);
        
        Logger::info(QStringLiteral("Decreasing cache size due to low hit rate"), {
            {"oldSize", m_maxSize / 0.8},
            {"newSize", m_maxSize},
            {"hitRate", hitRate}
        });
    }
}

void CacheManager::setMaxSize(int maxSize)
{
    QMutexLocker locker(&m_mutex);
    
    // 确保最大缓存大小在合理范围内
    const int MIN_CACHE_SIZE = 100;
    const int MAX_CACHE_SIZE = 100000;
    
    m_maxSize = qBound(MIN_CACHE_SIZE, maxSize, MAX_CACHE_SIZE);
    m_stats.maxSize = m_maxSize;
    
    // 如果当前缓存大小超过新的最大值，清理多余的条目
    while (m_cache.size() > m_maxSize) {
        evictLeastRecentlyUsed();
    }
}

int CacheManager::getMaxSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxSize;
}

void CacheManager::preloadCommonQueries(const QStringList& statementIds, QSharedPointer<Session> session)
{
    if (!m_enabled || !session) {
        return;
    }
    
    Logger::info(QStringLiteral("Preloading common queries into cache"), {
        {"queryCount", statementIds.size()}
    });
    
    int successCount = 0;
    int failureCount = 0;
    
    for (const QString& statementId : statementIds) {
        try {
            // 执行查询并自动存入缓存
            QVariant result = session->selectOne(statementId);
            if (!result.isNull()) {
                successCount++;
            } else {
                // 尝试作为列表查询
                QVariantList listResult = session->selectList(statementId);
                if (!listResult.isEmpty()) {
                    successCount++;
                } else {
                    failureCount++;
                }
            }
        } catch (const QtMyBatisException& e) {
            Logger::warn(QStringLiteral("Failed to preload cache for query"), {
                {"statementId", statementId},
                {"error", e.message()},
                {"code", e.code()}
            });
            failureCount++;
        } catch (const std::exception& e) {
            Logger::warn(QStringLiteral("Unexpected error preloading cache"), {
                {"statementId", statementId},
                {QStringLiteral("error"), QString::fromUtf8(e.what())}
            });
            failureCount++;
        }
    }
    
    Logger::info(QStringLiteral("Cache preloading completed"), {
        {"successCount", successCount},
        {"failureCount", failureCount}
    });
}

} // namespace QtMyBatisORM