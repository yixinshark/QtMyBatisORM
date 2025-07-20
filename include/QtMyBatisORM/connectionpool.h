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
 * 连接信息结构，用于跟踪连接的空闲时间和统计信息
 */
struct ConnectionInfo
{
    QSharedPointer<QSqlDatabase> connection;
    QDateTime lastUsedTime;
    QDateTime createdTime;
    QDateTime lastValidationTime; // 添加上次验证时间字段
    int usageCount;
    bool isHealthy;
    
    ConnectionInfo() = default;
    ConnectionInfo(QSharedPointer<QSqlDatabase> conn) 
        : connection(conn), 
          lastUsedTime(QDateTime::currentDateTime()),
          createdTime(QDateTime::currentDateTime()),
          lastValidationTime(), // 初始为无效时间，强制首次验证
          usageCount(0),
          isHealthy(true) {}
};

/**
 * 数据库连接池
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
    
    // 监控和健康检查方法
    ConnectionPoolStats getStats() const;
    ConnectionPoolHealth getHealthReport() const;
    void performHealthCheck();
    void resetStats();
    
    // 连接池优化方法
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
    
    // 统计信息
    mutable ConnectionPoolStats m_stats;
    QHash<QSharedPointer<QSqlDatabase>, ConnectionInfo> m_connectionInfoMap;
};

} // namespace QtMyBatisORM