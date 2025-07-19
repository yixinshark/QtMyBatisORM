#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>
#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestConnectionPoolMonitoring : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 监控功能测试
    void testGetStats();
    void testConnectionStatistics();
    void testHealthReport();
    void testPerformHealthCheck();
    void testResetStats();
    
    // 异常处理测试
    void testConnectionPoolExhausted();
    void testClosedPoolException();
    void testInvalidConnectionHandling();
    void testConnectionFailureStats();

private:
    DatabaseConfig createTestConfig();
    DatabaseConfig createInvalidConfig();
};

void TestConnectionPoolMonitoring::initTestCase()
{
    // 确保SQLite驱动可用
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QSKIP("SQLite driver not available");
    }
}

void TestConnectionPoolMonitoring::cleanupTestCase()
{
}

void TestConnectionPoolMonitoring::init()
{
}

void TestConnectionPoolMonitoring::cleanup()
{
}

DatabaseConfig TestConnectionPoolMonitoring::createTestConfig()
{
    DatabaseConfig config;
    config.driverName = "QSQLITE";
    config.databaseName = ":memory:";
    config.maxConnections = 5;
    config.minConnections = 2;
    config.maxIdleTime = 10; // 10秒空闲时间
    return config;
}

DatabaseConfig TestConnectionPoolMonitoring::createInvalidConfig()
{
    DatabaseConfig config;
    config.driverName = "INVALID_DRIVER";
    config.databaseName = "/invalid/path/database.db";
    config.maxConnections = 5;
    config.minConnections = 2;
    config.maxIdleTime = 10;
    return config;
}

void TestConnectionPoolMonitoring::testGetStats()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取初始统计信息
    ConnectionPoolStats stats = pool.getStats();
    
    // 验证初始状态
    QCOMPARE(stats.totalConnections, 2); // minConnections
    QCOMPARE(stats.availableConnections, 2);
    QCOMPARE(stats.usedConnections, 0);
    QCOMPARE(stats.totalConnectionsCreated, 2);
    QCOMPARE(stats.totalConnectionsDestroyed, 0);
    QCOMPARE(stats.connectionFailures, 0);
    QCOMPARE(stats.connectionTimeouts, 0);
    QCOMPARE(stats.peakUsedConnections, 0);
    
    // 获取一个连接
    auto conn = pool.getConnection();
    QVERIFY(conn != nullptr);
    
    // 再次获取统计信息
    stats = pool.getStats();
    QCOMPARE(stats.totalConnections, 2);
    QCOMPARE(stats.availableConnections, 1);
    QCOMPARE(stats.usedConnections, 1);
    QCOMPARE(stats.peakUsedConnections, 1);
    
    // 归还连接
    pool.returnConnection(conn);
    
    // 验证归还后的统计
    stats = pool.getStats();
    QCOMPARE(stats.totalConnections, 2);
    QCOMPARE(stats.availableConnections, 2);
    QCOMPARE(stats.usedConnections, 0);
}

void TestConnectionPoolMonitoring::testConnectionStatistics()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取多个连接以测试峰值统计
    QList<QSharedPointer<QSqlDatabase>> connections;
    
    for (int i = 0; i < 3; ++i) {
        auto conn = pool.getConnection();
        QVERIFY(conn != nullptr);
        connections.append(conn);
    }
    
    ConnectionPoolStats stats = pool.getStats();
    QCOMPARE(stats.usedConnections, 3);
    QCOMPARE(stats.peakUsedConnections, 3);
    QVERIFY(stats.totalConnectionsCreated >= 3);
    
    // 归还所有连接
    for (auto conn : connections) {
        pool.returnConnection(conn);
    }
    
    stats = pool.getStats();
    QCOMPARE(stats.usedConnections, 0);
    QCOMPARE(stats.peakUsedConnections, 3); // 峰值应该保持
}

void TestConnectionPoolMonitoring::testHealthReport()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取健康报告
    ConnectionPoolHealth health = pool.getHealthReport();
    
    // 验证健康状态
    QCOMPARE(health.overallHealth, ConnectionHealth::HEALTHY);
    QVERIFY(health.healthyConnections >= 2);
    QCOMPARE(health.unhealthyConnections, 0);
    QCOMPARE(health.failedHealthChecks, 0);
    QVERIFY(!health.healthMessage.isEmpty());
    QVERIFY(health.errors.isEmpty());
    
    // 验证时间戳
    QVERIFY(health.lastHealthCheck.isValid());
    QVERIFY(health.lastHealthCheck <= QDateTime::currentDateTime());
}

void TestConnectionPoolMonitoring::testPerformHealthCheck()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 执行健康检查
    pool.performHealthCheck();
    
    // 验证健康检查后的状态
    ConnectionPoolHealth health = pool.getHealthReport();
    QCOMPARE(health.overallHealth, ConnectionHealth::HEALTHY);
    
    // 获取连接并执行健康检查
    auto conn = pool.getConnection();
    QVERIFY(conn != nullptr);
    
    pool.performHealthCheck();
    
    health = pool.getHealthReport();
    QVERIFY(health.healthyConnections > 0);
    
    pool.returnConnection(conn);
}

void TestConnectionPoolMonitoring::testResetStats()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取一些连接以产生统计数据
    auto conn1 = pool.getConnection();
    auto conn2 = pool.getConnection();
    
    ConnectionPoolStats statsBefore = pool.getStats();
    QVERIFY(statsBefore.usedConnections > 0);
    QVERIFY(statsBefore.peakUsedConnections > 0);
    
    // 重置统计信息
    pool.resetStats();
    
    ConnectionPoolStats statsAfter = pool.getStats();
    
    // 验证当前连接数保持不变
    QCOMPARE(statsAfter.totalConnections, statsBefore.totalConnections);
    QCOMPARE(statsAfter.availableConnections, statsBefore.availableConnections);
    QCOMPARE(statsAfter.usedConnections, statsBefore.usedConnections);
    
    // 验证其他统计信息被重置
    QCOMPARE(statsAfter.totalConnectionsCreated, 0);
    QCOMPARE(statsAfter.totalConnectionsDestroyed, 0);
    QCOMPARE(statsAfter.connectionFailures, 0);
    QCOMPARE(statsAfter.connectionTimeouts, 0);
    QCOMPARE(statsAfter.peakUsedConnections, 0);
    
    pool.returnConnection(conn1);
    pool.returnConnection(conn2);
}

void TestConnectionPoolMonitoring::testConnectionPoolExhausted()
{
    DatabaseConfig config = createTestConfig();
    config.maxConnections = 2; // 设置较小的最大连接数
    ConnectionPool pool(config);
    
    QList<QSharedPointer<QSqlDatabase>> connections;
    
    // 获取所有可用连接
    for (int i = 0; i < config.maxConnections; ++i) {
        auto conn = pool.getConnection();
        QVERIFY(conn != nullptr);
        connections.append(conn);
    }
    
    // 尝试获取超出限制的连接，应该抛出异常
    bool exceptionThrown = false;
    try {
        auto conn = pool.getConnection();
    } catch (const ConnectionException& e) {
        exceptionThrown = true;
        QVERIFY(e.message().contains("exhausted"));
    }
    QVERIFY(exceptionThrown);
    
    // 验证统计信息
    ConnectionPoolStats stats = pool.getStats();
    QVERIFY(stats.connectionTimeouts > 0);
    QCOMPARE(stats.maxConnectionsReached, config.maxConnections);
    
    // 归还连接
    for (auto conn : connections) {
        pool.returnConnection(conn);
    }
}

void TestConnectionPoolMonitoring::testClosedPoolException()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 关闭连接池
    pool.close();
    
    // 尝试从已关闭的连接池获取连接，应该抛出异常
    bool exceptionThrown = false;
    try {
        auto conn = pool.getConnection();
    } catch (const ConnectionException& e) {
        exceptionThrown = true;
        QVERIFY(e.message().contains("closed"));
    }
    QVERIFY(exceptionThrown);
}

void TestConnectionPoolMonitoring::testInvalidConnectionHandling()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取连接
    auto conn = pool.getConnection();
    QVERIFY(conn != nullptr);
    
    // 手动关闭连接以模拟无效连接
    if (conn->isOpen()) {
        conn->close();
    }
    
    // 归还无效连接
    pool.returnConnection(conn);
    
    // 验证连接池能正确处理无效连接
    ConnectionPoolStats stats = pool.getStats();
    QVERIFY(stats.totalConnectionsDestroyed > 0);
}

void TestConnectionPoolMonitoring::testConnectionFailureStats()
{
    // 使用无效配置创建连接池
    DatabaseConfig config = createInvalidConfig();
    
    // 创建连接池时应该记录连接失败
    ConnectionPool pool(config);
    
    // 验证连接失败统计
    ConnectionPoolStats stats = pool.getStats();
    QVERIFY(stats.connectionFailures > 0);
    
    // 尝试获取连接也应该失败
    bool exceptionThrown = false;
    try {
        auto conn = pool.getConnection();
    } catch (const ConnectionException& e) {
        exceptionThrown = true;
    }
    
    // 验证异常确实被抛出
    QVERIFY(exceptionThrown);
    
    // 验证失败统计增加
    stats = pool.getStats();
    QVERIFY(stats.connectionFailures > 0);
}

QTEST_MAIN(TestConnectionPoolMonitoring)
#include "run_connectionpool_monitoring_test.moc"