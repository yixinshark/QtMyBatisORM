#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/logger.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMutexLocker>
#include <QTimer>
#include <QUuid>
#include <QThread>

namespace QtMyBatisORM {

ConnectionPool::ConnectionPool(const DatabaseConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_connectionCounter(0)
    , m_closed(false)
{
    // 初始化统计信息
    m_stats = ConnectionPoolStats();
    
    // 设置清理定时器，每30秒清理一次空闲连接
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &ConnectionPool::cleanupIdleConnections);
    m_cleanupTimer->start(30000); // 每30秒清理一次
    
    // 创建最小连接数
    for (int i = 0; i < m_config.minConnections; ++i) {
        try {
            QSharedPointer<QSqlDatabase> conn = createConnection();
            if (conn) {
                ConnectionInfo connInfo(conn);
                m_availableConnections.enqueue(connInfo);
                m_connectionInfoMap[conn] = connInfo;
                
                // 更新统计信息
                m_stats.totalConnectionsCreated++;
                m_stats.totalConnections++;
                m_stats.availableConnections++;
                m_stats.lastConnectionCreated = QDateTime::currentDateTime();
            }
        } catch (const ConnectionException& e) {
            m_stats.connectionFailures++;
            // 记录错误但继续创建其他连接
        }
    }
}

ConnectionPool::~ConnectionPool()
{
    close();
}

QSharedPointer<QSqlDatabase> ConnectionPool::getConnection()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_closed) {
        Logger::error(QStringLiteral("Connection pool is closed"), {
            {QStringLiteral("poolState"), QStringLiteral("closed")},
            {"totalConnections", m_stats.totalConnections}
        });
        
        ConnectionException ex("Connection pool is closed", "POOL_CLOSED");
        ex.setContext(QStringLiteral("poolState"), "closed");
        ex.setContext(QStringLiteral("totalConnections"), m_stats.totalConnections);
        throw ex;
    }
    
    QSharedPointer<QSqlDatabase> connection;
    int retryCount = 0;
    const int maxRetries = 3;
    
    // 尝试从可用连接中获取
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo connInfo = m_availableConnections.dequeue();
        connection = connInfo.connection;
        
        try {
            if (isConnectionValid(connection)) {
                m_usedConnections.insert(connection);
                
                // 更新连接信息和统计
                if (m_connectionInfoMap.contains(connection)) {
                    m_connectionInfoMap[connection].lastUsedTime = QDateTime::currentDateTime();
                    m_connectionInfoMap[connection].usageCount++;
                }
                
                // 更新统计信息
                m_stats.availableConnections--;
                m_stats.usedConnections++;
                if (m_stats.usedConnections > m_stats.peakUsedConnections) {
                    m_stats.peakUsedConnections = m_stats.usedConnections;
                }
                
                return connection;
            } else {
                removeConnection(connection);
            }
        } catch (const std::exception& e) {
            qWarning() << "Error validating connection:" << e.what();
            removeConnection(connection);
        }
    }
    
    // 如果没有可用连接且未达到最大连接数，创建新连接
    if (m_usedConnections.size() < m_config.maxConnections) {
        while (retryCount < maxRetries) {
            try {
                connection = createConnection();
                if (connection) {
                    m_usedConnections.insert(connection);
                    
                    // 创建连接信息
                    ConnectionInfo connInfo(connection);
                    m_connectionInfoMap[connection] = connInfo;
                    
                    // 更新统计信息
                    m_stats.totalConnectionsCreated++;
                    m_stats.totalConnections++;
                    m_stats.usedConnections++;
                    m_stats.lastConnectionCreated = QDateTime::currentDateTime();
                    if (m_stats.usedConnections > m_stats.peakUsedConnections) {
                        m_stats.peakUsedConnections = m_stats.usedConnections;
                    }
                    
                    return connection;
                }
            } catch (const ConnectionException& e) {
                m_stats.connectionFailures++;
                retryCount++;
                
                if (retryCount >= maxRetries) {
                    ConnectionException ex(
                        QStringLiteral("Failed to create connection after %1 retries: %2")
                        .arg(maxRetries).arg(e.message()),
                        "CONNECTION_CREATE_FAILED"
                    );
                    ex.setContext(QStringLiteral("retryCount"), retryCount);
                    ex.setContext(QStringLiteral("maxRetries"), maxRetries);
                    ex.setContext(QStringLiteral("originalError"), e.message());
                    ex.setContext(QStringLiteral("originalCode"), e.code());
                    ex.setContext(QStringLiteral("poolStats"), QVariant::fromValue(m_stats));
                    throw ex;
                }
                
                // 等待一段时间后重试
                QThread::msleep(100 * retryCount); // 递增延迟
            } catch (const std::exception& e) {
                m_stats.connectionFailures++;
                ConnectionException ex(
                    QStringLiteral("Unexpected error creating connection: %1").arg(QString::fromUtf8(e.what())),
                    "CONNECTION_UNEXPECTED_ERROR"
                );
                ex.setContext(QStringLiteral("stdError"), e.what());
                ex.setContext(QStringLiteral("retryCount"), retryCount);
                throw ex;
            }
        }
    }
    
    // 连接池已满
    m_stats.connectionTimeouts++;
    if (m_stats.usedConnections > m_stats.maxConnectionsReached) {
        m_stats.maxConnectionsReached = m_stats.usedConnections;
    }
    
    ConnectionException ex("Connection pool exhausted", "POOL_EXHAUSTED");
    ex.setContext(QStringLiteral("maxConnections"), m_config.maxConnections);
    ex.setContext(QStringLiteral("usedConnections"), m_stats.usedConnections);
    ex.setContext(QStringLiteral("availableConnections"), m_stats.availableConnections);
    ex.setContext(QStringLiteral("totalConnections"), m_stats.totalConnections);
    ex.setContext(QStringLiteral("connectionTimeouts"), m_stats.connectionTimeouts);
    throw ex;
}

void ConnectionPool::returnConnection(QSharedPointer<QSqlDatabase> connection)
{
    QMutexLocker locker(&m_mutex);
    
    if (!connection || m_closed) {
        return;
    }
    
    if (m_usedConnections.contains(connection)) {
        m_usedConnections.remove(connection);
        
        if (isConnectionValid(connection)) {
            // 更新连接信息
            if (m_connectionInfoMap.contains(connection)) {
                ConnectionInfo connInfo = m_connectionInfoMap[connection];
                connInfo.lastUsedTime = QDateTime::currentDateTime();
                m_availableConnections.enqueue(connInfo);
                m_connectionInfoMap[connection] = connInfo;
            } else {
                ConnectionInfo connInfo(connection);
                m_availableConnections.enqueue(connInfo);
                m_connectionInfoMap[connection] = connInfo;
            }
            
            // 更新统计信息
            m_stats.usedConnections--;
            m_stats.availableConnections++;
        } else {
            removeConnection(connection);
        }
    }
}

void ConnectionPool::close()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_closed) {
        return;
    }
    
    m_closed = true;
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
    
    // 关闭所有连接
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo connInfo = m_availableConnections.dequeue();
        QSharedPointer<QSqlDatabase> conn = connInfo.connection;
        if (conn && conn->isOpen()) {
            qDebug() << "close connection:" << conn->connectionName() << conn.data();
            conn->close();
        }
    }
    
    for (auto conn : m_usedConnections) {
        if (conn && conn->isOpen()) {
            conn->close();
        }
    }
    m_usedConnections.clear();
}

int ConnectionPool::availableConnections() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_availableConnections.size();
}

int ConnectionPool::usedConnections() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_usedConnections.size();
}

int ConnectionPool::totalConnections() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_availableConnections.size() + m_usedConnections.size();
}

void ConnectionPool::cleanupIdleConnections()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_closed) {
        return;
    }
    
    // 清理超时的空闲连接，但保持最小连接数
    QQueue<ConnectionInfo> validConnections;
    
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo connInfo = m_availableConnections.dequeue();
        
        // 如果连接已经超时且超过最小连接数，则移除
        if (isConnectionIdle(connInfo) && validConnections.size() >= m_config.minConnections) {
            removeConnection(connInfo.connection);
        } else {
            validConnections.enqueue(connInfo);
        }
    }
    
    m_availableConnections = validConnections;
}

QSharedPointer<QSqlDatabase> ConnectionPool::createConnection()
{
    QString connectionName = QStringLiteral("QtMyBatisORM_%1_%2")
                            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
                            .arg(++m_connectionCounter);
    
    QSqlDatabase db = QSqlDatabase::addDatabase(m_config.driverName, connectionName);
    
    // 配置数据库连接参数
    if (m_config.driverName == QStringLiteral("QMYSQL")) {
        db.setHostName(m_config.hostName);
        db.setPort(m_config.port);
        db.setUserName(m_config.userName);
        db.setPassword(m_config.password);
        db.setDatabaseName(m_config.databaseName);
    } else if (m_config.driverName == QStringLiteral("QSQLITE")) {
        db.setDatabaseName(m_config.databaseName);
        // SQLite特定配置
        if (!m_config.userName.isEmpty()) {
            db.setUserName(m_config.userName);
        }
        if (!m_config.password.isEmpty()) {
            db.setPassword(m_config.password);
        }
    } else {
        // 通用配置
        if (!m_config.hostName.isEmpty()) {
            db.setHostName(m_config.hostName);
        }
        if (m_config.port > 0) {
            db.setPort(m_config.port);
        }
        if (!m_config.userName.isEmpty()) {
            db.setUserName(m_config.userName);
        }
        if (!m_config.password.isEmpty()) {
            db.setPassword(m_config.password);
        }
        db.setDatabaseName(m_config.databaseName);
    }
    
    if (!db.open()) {
        QString errorMsg = QStringLiteral("Failed to open database connection [%1]: %2")
                          .arg(m_config.driverName)
                          .arg(db.lastError().text());
        QSqlDatabase::removeDatabase(connectionName);
        throw ConnectionException(errorMsg);
    }
    
    // 创建一个指向QSqlDatabase的智能指针
    // 注意：QSqlDatabase不能直接用QSharedPointer包装，需要特殊处理
    auto dbPtr = QSharedPointer<QSqlDatabase>(new QSqlDatabase(db), 
        [connectionName](QSqlDatabase* db) {
            if (db && db->isOpen()) {
                db->close();
            }
            delete db;
            QSqlDatabase::removeDatabase(connectionName);
        });
    
    return dbPtr;
}

bool ConnectionPool::isConnectionValid(QSharedPointer<QSqlDatabase> connection)
{
    if (!connection) {
        return false;
    }
    
    // 首先检查连接状态，无需执行查询
    if (!connection->isOpen() || !connection->isValid()) {
        return false;
    }
    
    // 获取连接信息
    if (!m_connectionInfoMap.contains(connection)) {
        m_connectionInfoMap[connection] = ConnectionInfo(connection);
    }
    
    ConnectionInfo& info = m_connectionInfoMap[connection];
    QDateTime now = QDateTime::currentDateTime();
    
    // 只有在以下情况下才执行完整检查：
    // 1. 上次错误表明可能存在连接问题
    // 2. 连接长时间未使用
    // 3. 连接从未进行过验证
    const int VALIDATION_INTERVAL_SECONDS = 60; // 每分钟最多验证一次
    bool needsFullCheck = connection->lastError().isValid() || 
                         !info.lastValidationTime.isValid() ||
                         info.lastValidationTime.secsTo(now) > VALIDATION_INTERVAL_SECONDS;
    
    if (needsFullCheck) {
        try {
            QSqlQuery query(*connection);
            bool valid = query.exec(QStringLiteral("SELECT 1"));
            
            // 更新验证时间
            info.lastValidationTime = now;
            info.isHealthy = valid;
            
            return valid;
        } catch (...) {
            info.isHealthy = false;
            return false;
        }
    }
    
    // 如果不需要完整检查，返回上次检查的健康状态
    return info.isHealthy;
}

void ConnectionPool::removeConnection(QSharedPointer<QSqlDatabase> connection)
{
    if (connection && connection->isOpen()) {
        QString connectionName = connection->connectionName();
        connection->close();
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    // 从连接信息映射中移除
    if (m_connectionInfoMap.contains(connection)) {
        m_connectionInfoMap.remove(connection);
    }
    
    // 更新统计信息
    m_stats.totalConnectionsDestroyed++;
    m_stats.lastConnectionDestroyed = QDateTime::currentDateTime();
    m_stats.totalConnections--;
}

bool ConnectionPool::isConnectionIdle(const ConnectionInfo& connInfo) const
{
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 idleSeconds = connInfo.lastUsedTime.secsTo(currentTime);
    return idleSeconds > m_config.maxIdleTime;
}

ConnectionPoolStats ConnectionPool::getStats() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    
    // 更新当前状态统计
    m_stats.totalConnections = m_availableConnections.size() + m_usedConnections.size();
    m_stats.availableConnections = m_availableConnections.size();
    m_stats.usedConnections = m_usedConnections.size();
    
    // 计算平均连接存活时间
    if (!m_connectionInfoMap.isEmpty()) {
        QDateTime currentTime = QDateTime::currentDateTime();
        double totalAge = 0.0;
        int validConnections = 0;
        
        for (auto it = m_connectionInfoMap.constBegin(); it != m_connectionInfoMap.constEnd(); ++it) {
            const ConnectionInfo& info = it.value();
            double age = info.createdTime.secsTo(currentTime);
            if (age > 0) {
                totalAge += age;
                validConnections++;
            }
        }
        
        if (validConnections > 0) {
            m_stats.averageConnectionAge = totalAge / validConnections;
        }
    }
    
    return m_stats;
}

ConnectionPoolHealth ConnectionPool::getHealthReport() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    
    ConnectionPoolHealth health;
    health.lastHealthCheck = QDateTime::currentDateTime();
    
    // 检查所有连接的健康状态
    int healthyCount = 0;
    int unhealthyCount = 0;
    
    // 检查可用连接
    for (const ConnectionInfo& connInfo : m_availableConnections) {
        if (const_cast<ConnectionPool*>(this)->isConnectionValid(connInfo.connection)) {
            healthyCount++;
        } else {
            unhealthyCount++;
        }
    }
    
    // 检查正在使用的连接
    for (auto conn : m_usedConnections) {
        if (m_connectionInfoMap.contains(conn)) {
            const ConnectionInfo& info = m_connectionInfoMap[conn];
            if (info.isHealthy) {
                healthyCount++;
            } else {
                unhealthyCount++;
            }
        }
    }
    
    health.healthyConnections = healthyCount;
    health.unhealthyConnections = unhealthyCount;
    health.failedHealthChecks = m_stats.connectionFailures;
    
    // 确定整体健康状态
    int totalConnections = healthyCount + unhealthyCount;
    if (totalConnections == 0) {
        health.overallHealth = ConnectionHealth::UNKNOWN;
        health.healthMessage = QStringLiteral("No connections available");
        health.warnings.append(QStringLiteral("Connection pool has no active connections"));
    } else {
        double healthRatio = static_cast<double>(healthyCount) / totalConnections;
        
        if (healthRatio >= 0.9) {
            health.overallHealth = ConnectionHealth::HEALTHY;
            health.healthMessage = QStringLiteral("Connection pool is healthy");
        } else if (healthRatio >= 0.7) {
            health.overallHealth = ConnectionHealth::DEGRADED;
            health.healthMessage = QStringLiteral("Connection pool performance is degraded");
            health.warnings.append(QStringLiteral("Only %1% of connections are healthy").arg(healthRatio * 100, 0, 'f', 1));
        } else {
            health.overallHealth = ConnectionHealth::UNHEALTHY;
            health.healthMessage = QStringLiteral("Connection pool is unhealthy");
            health.errors.append(QStringLiteral("Only %1% of connections are healthy").arg(healthRatio * 100, 0, 'f', 1));
        }
    }
    
    // 检查连接池使用率
    if (m_stats.usedConnections >= m_config.maxConnections * 0.9) {
        health.warnings.append(QStringLiteral("Connection pool usage is high (>90%)"));
    }
    
    // 检查连接失败率
    if (m_stats.connectionFailures > 0) {
        health.warnings.append(QStringLiteral("Connection failures detected: %1").arg(m_stats.connectionFailures));
    }
    
    // 检查连接超时
    if (m_stats.connectionTimeouts > 0) {
        health.warnings.append(QStringLiteral("Connection timeouts detected: %1").arg(m_stats.connectionTimeouts));
    }
    
    return health;
}

void ConnectionPool::performHealthCheck()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_closed) {
        return;
    }
    
    // 检查所有可用连接的健康状态
    QQueue<ConnectionInfo> healthyConnections;
    
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo connInfo = m_availableConnections.dequeue();
        
        if (isConnectionValid(connInfo.connection)) {
            connInfo.isHealthy = true;
            healthyConnections.enqueue(connInfo);
            m_connectionInfoMap[connInfo.connection] = connInfo;
        } else {
            connInfo.isHealthy = false;
            m_connectionInfoMap[connInfo.connection] = connInfo;
            removeConnection(connInfo.connection);
            m_connectionInfoMap.remove(connInfo.connection);
            
            // 更新统计信息
            m_stats.totalConnectionsDestroyed++;
            m_stats.lastConnectionDestroyed = QDateTime::currentDateTime();
            m_stats.totalConnections--;
            m_stats.availableConnections--;
        }
    }
    
    m_availableConnections = healthyConnections;
    
    // 更新正在使用连接的健康状态
    for (auto conn : m_usedConnections) {
        if (m_connectionInfoMap.contains(conn)) {
            ConnectionInfo& info = m_connectionInfoMap[conn];
            info.isHealthy = isConnectionValid(conn);
        }
    }
}

void ConnectionPool::resetStats()
{
    QMutexLocker locker(&m_mutex);
    
    // 保留当前连接数量信息，重置其他统计
    int currentTotal = m_stats.totalConnections;
    int currentAvailable = m_stats.availableConnections;
    int currentUsed = m_stats.usedConnections;
    
    m_stats = ConnectionPoolStats();
    m_stats.totalConnections = currentTotal;
    m_stats.availableConnections = currentAvailable;
    m_stats.usedConnections = currentUsed;
}

void ConnectionPool::monitorConnectionUsage()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_closed) {
        return;
    }
    
    // Calculate connection usage trend
    int totalConnections = m_availableConnections.size() + m_usedConnections.size();
    double usageRatio = static_cast<double>(m_usedConnections.size()) / 
                       (totalConnections > 0 ? totalConnections : 1);
    
    // If usage is high, proactively create more connections
    if (usageRatio > 0.8 && totalConnections < m_config.maxConnections) {
        int connectionsToCreate = qMin(
            m_config.maxConnections - totalConnections,
            static_cast<int>(totalConnections * 0.2) // Create up to 20% more
        );
        
        Logger::info(QStringLiteral("Proactively creating connections due to high usage"), {
            {"usageRatio", usageRatio},
            {"currentConnections", totalConnections},
            {"connectionsToCreate", connectionsToCreate}
        });
        
        for (int i = 0; i < connectionsToCreate; i++) {
            try {
                QSharedPointer<QSqlDatabase> conn = createConnection();
                if (conn) {
                    ConnectionInfo connInfo(conn);
                    m_availableConnections.enqueue(connInfo);
                    m_connectionInfoMap[conn] = connInfo;
                    
                    // Update statistics
                    m_stats.totalConnectionsCreated++;
                    m_stats.totalConnections++;
                    m_stats.availableConnections++;
                    m_stats.lastConnectionCreated = QDateTime::currentDateTime();
                }
            } catch (const ConnectionException& e) {
                Logger::warn(QStringLiteral("Failed to create proactive connection"), {
                    {"error", e.message()},
                    {"code", e.code()}
                });
                // Stop if we can't create more
                break;
            } catch (...) {
                Logger::warn(QStringLiteral("Unexpected error creating proactive connection"));
                // Stop if we can't create more
                break;
            }
        }
    }
}

} // namespace QtMyBatisORM