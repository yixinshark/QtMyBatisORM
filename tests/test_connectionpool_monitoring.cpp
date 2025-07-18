#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QElapsedTimer>
#include <QThread>
#include <QTemporaryDir>

#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/logger.h"

using namespace QtMyBatisORM;

class TestConnectionPoolMonitoring : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 测试连接池监控功能
    void testConnectionPoolMonitoring();
    
    // 测试连接池预测性创建
    void testPredictiveConnectionCreation();
    
    // 测试连接池健康检查
    void testConnectionPoolHealthCheck();
    
    // 测试连接池统计信息
    void testConnectionPoolStats();
    
    // 测试连接池优化
    void testConnectionPoolOptimization();

private:
    QTemporaryDir m_tempDir;
    QString m_dbPath;
    QSharedPointer<ConnectionPool> m_pool;
    DatabaseConfig m_config;
};

void TestConnectionPoolMonitoring::initTestCase()
{
    // 创建临时目录用于测试文件
    QVERIFY(m_tempDir.isValid());
    
    // 设置数据库路径
    m_dbPath = m_tempDir.path() + "/test.db";
    
    // 创建数据库配置
    m_config.driverName = "QSQLITE";
    m_config.databaseName = m_dbPath;
    m_config.maxConnections = 10;
    m_config.minConnections = 2;
    m_config.maxIdleTime = 60;
    
    // 创建连接池
    m_pool = QSharedPointer<ConnectionPool>::create(m_config);
    QVERIFY(m_pool != nullptr);
    
    // 设置日志级别为DEBUG以便查看详细信息
    Logger::setLogLevel(LogLevel::DEBUG_LEVEL);
}

void TestConnectionPoolMonitoring::cleanupTestCase()
{
    m_pool->close();
    m_pool.reset();
}

void TestConnectionPoolMonitoring::init()
{
    // 每个测试前的准备工作
}

void TestConnectionPoolMonitoring::cleanup()
{
    // 每个测试后的清理工作
}

void TestConnectionPoolMonitoring::testConnectionPoolMonitoring()
{
    // 获取初始连接池状态
    ConnectionPoolStats initialStats = m_pool->getStats();
    
    // 创建一些连接
    QList<QSharedPointer<QSqlDatabase>> connections;
    for (int i = 0; i < 5; ++i) {
        connections.append(m_pool->getConnection());
    }
    
    // 验证连接池状态已更新
    ConnectionPoolStats currentStats = m_pool->getStats();
    QCOMPARE(currentStats.usedConnections, 5);
    QVERIFY(currentStats.totalConnectionsCreated >= initialStats.totalConnectionsCreated);
    
    // 返回连接到池中
    for (auto& conn : connections) {
        m_pool->returnConnection(conn);
    }
    connections.clear();
    
    // 验证连接已返回到池中
    currentStats = m_pool->getStats();
    QCOMPARE(currentStats.usedConnections, 0);
    QVERIFY(currentStats.availableConnections >= 5);
    
    // 执行连接池监控
    m_pool->monitorConnectionUsage();
    
    // 验证监控不会导致错误
    currentStats = m_pool->getStats();
    QVERIFY(currentStats.connectionFailures == 0);
}

void TestConnectionPoolMonitoring::testPredictiveConnectionCreation()
{
    // 获取初始连接池状态
    ConnectionPoolStats initialStats = m_pool->getStats();
    int initialTotal = initialStats.totalConnections;
    
    // 创建大量连接以触发预测性创建
    QList<QSharedPointer<QSqlDatabase>> connections;
    for (int i = 0; i < 8; ++i) {
        connections.append(m_pool->getConnection());
    }
    
    // 执行连接池监控以触发预测性创建
    m_pool->monitorConnectionUsage();
    
    // 返回连接到池中
    for (auto& conn : connections) {
        m_pool->returnConnection(conn);
    }
    connections.clear();
    
    // 验证连接池可能已经预创建了额外的连接
    ConnectionPoolStats currentStats = m_pool->getStats();
    QVERIFY(currentStats.totalConnections >= initialTotal);
}

void TestConnectionPoolMonitoring::testConnectionPoolHealthCheck()
{
    // 执行健康检查
    m_pool->performHealthCheck();
    
    // 获取健康报告
    ConnectionPoolHealth health = m_pool->getHealthReport();
    
    // 验证健康报告包含有效数据
    QVERIFY(health.lastHealthCheck.isValid());
    QVERIFY(health.healthyConnections >= 0);
    QVERIFY(health.unhealthyConnections >= 0);
    
    // 验证总体健康状态是有效的枚举值
    QVERIFY(health.overallHealth == ConnectionHealth::HEALTHY || 
            health.overallHealth == ConnectionHealth::DEGRADED || 
            health.overallHealth == ConnectionHealth::UNHEALTHY || 
            health.overallHealth == ConnectionHealth::UNKNOWN);
    
    // 验证健康消息不为空
    QVERIFY(!health.healthMessage.isEmpty());
}

void TestConnectionPoolMonitoring::testConnectionPoolStats()
{
    // 重置统计信息
    m_pool->resetStats();
    
    // 获取重置后的统计信息
    ConnectionPoolStats stats = m_pool->getStats();
    
    // 验证基本统计信息已重置
    QCOMPARE(stats.connectionFailures, 0);
    QCOMPARE(stats.connectionTimeouts, 0);
    
    // 创建一些连接
    QList<QSharedPointer<QSqlDatabase>> connections;
    for (int i = 0; i < 3; ++i) {
        connections.append(m_pool->getConnection());
    }
    
    // 验证统计信息已更新
    stats = m_pool->getStats();
    QCOMPARE(stats.usedConnections, 3);
    
    // 返回连接到池中
    for (auto& conn : connections) {
        m_pool->returnConnection(conn);
    }
    connections.clear();
    
    // 验证统计信息已更新
    stats = m_pool->getStats();
    QCOMPARE(stats.usedConnections, 0);
}

void TestConnectionPoolMonitoring::testConnectionPoolOptimization()
{
    // 获取初始连接池状态
    ConnectionPoolStats initialStats = m_pool->getStats();
    
    // 模拟高负载场景
    QList<QSharedPointer<QSqlDatabase>> connections;
    for (int i = 0; i < m_config.maxConnections - 1; ++i) {
        connections.append(m_pool->getConnection());
    }
    
    // 执行连接池监控以触发优化
    m_pool->monitorConnectionUsage();
    
    // 返回连接到池中
    for (auto& conn : connections) {
        m_pool->returnConnection(conn);
    }
    connections.clear();
    
    // 验证连接池状态
    ConnectionPoolStats currentStats = m_pool->getStats();
    QVERIFY(currentStats.totalConnections >= initialStats.totalConnections);
    
    // 执行健康检查
    m_pool->performHealthCheck();
    
    // 获取健康报告
    ConnectionPoolHealth health = m_pool->getHealthReport();
    
    // 验证健康状态
    QVERIFY(health.overallHealth == ConnectionHealth::HEALTHY || 
            health.overallHealth == ConnectionHealth::DEGRADED);
}

#include "test_connectionpool_monitoring.moc"