#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <QMutex>
#include <QSharedPointer>
#include <QDateTime>
#include <QHash>
#include "datamodels.h"

namespace QtMyBatisORM {

/**
 * Connection info structure for tracking connection idle time and statistics
 * 连接信息结构，用于跟踪连接的空闲时间和统计信息
 */
struct ConnectionInfo
{
    QSharedPointer<QSqlDatabase> connection;
    QDateTime lastUsedTime;
    QDateTime createdTime;
    QDateTime lastValidationTime; // Add last validation time field；添加上次验证时间字段
    int usageCount;
    bool isHealthy;
    
    ConnectionInfo() = default;
    ConnectionInfo(QSharedPointer<QSqlDatabase> conn) 
        : connection(conn), 
          lastUsedTime(QDateTime::currentDateTime()),
          createdTime(QDateTime::currentDateTime()),
          lastValidationTime(), // Initially invalid time, force first validation
          usageCount(0),
          isHealthy(true) {}
};

/**
 * Database connection pool
 */
class ConnectionPool : public QObject
{
    Q_OBJECT
    
public:
    explicit ConnectionPool(const DatabaseConfig& config, QObject* parent = nullptr);
    ~ConnectionPool();
    
    QSharedPointer<QSqlDatabase> getConnection();
    void returnConnection(QSharedPointer<QSqlDatabase> connection);
    
    void close();
    int availableConnections() const;
    int usedConnections() const;
    int totalConnections() const;
    
    // Monitoring and health check methods
    ConnectionPoolStats getStats() const;
    ConnectionPoolHealth getHealthReport() const;
    void performHealthCheck();
    void resetStats();
    
    // Connection pool optimization methods
    void monitorConnectionUsage();
    
private slots:
    void cleanupIdleConnections();
    
private:
    QSharedPointer<QSqlDatabase> createConnection();
    bool isConnectionValid(QSharedPointer<QSqlDatabase> connection);
    void removeConnection(QSharedPointer<QSqlDatabase> connection);
    bool isConnectionIdle(const ConnectionInfo& connInfo) const;
    
    DatabaseConfig m_config;
    QQueue<ConnectionInfo> m_availableConnections;
    QSet<QSharedPointer<QSqlDatabase>> m_usedConnections;
    QTimer* m_cleanupTimer;
    mutable QMutex m_mutex;
    
    int m_connectionCounter;
    bool m_closed;
    
    // Statistics information
    mutable ConnectionPoolStats m_stats;
    QHash<QSharedPointer<QSqlDatabase>, ConnectionInfo> m_connectionInfoMap;
};

} // namespace QtMyBatisORM