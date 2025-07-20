#include <QtTest>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/datamodels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestConnectionPool : public QObject
{
    Q_OBJECT

private slots:
    void testCreateConnectionPool();
    void testGetConnection();
    void testReturnConnection();
    void testConnectionPoolExhaustion();
    void testConnectionValidation();
    void testConnectionPoolClose();
    void testIdleConnectionCleanup();
    void testConcurrentAccess();
    void testThreadSafety();

private:
    DatabaseConfig createTestConfig();
};

DatabaseConfig TestConnectionPool::createTestConfig()
{
    DatabaseConfig config;
    config.driverName = "QSQLITE";
    config.databaseName = ":memory:";
    config.maxConnections = 3;
    config.minConnections = 1;
    return config;
}

void TestConnectionPool::testCreateConnectionPool()
{
    DatabaseConfig config = createTestConfig();
    
    ConnectionPool pool(config);
    
    // 验证初始状态
    QCOMPARE(pool.availableConnections(), 1);
    QCOMPARE(pool.usedConnections(), 0);
    QCOMPARE(pool.totalConnections(), 1);
}

void TestConnectionPool::testGetConnection()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取连接
    auto connection = pool.getConnection();
    QVERIFY(connection != nullptr);
    QVERIFY(connection->isOpen());
    
    // 验证连接池状态
    QCOMPARE(pool.availableConnections(), 0);
    QCOMPARE(pool.usedConnections(), 1);
    
    // 归还连接
    pool.returnConnection(connection);
    QCOMPARE(pool.availableConnections(), 1);
    QCOMPARE(pool.usedConnections(), 0);
}

void TestConnectionPool::testReturnConnection()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    // 获取多个连接
    auto conn1 = pool.getConnection();
    auto conn2 = pool.getConnection();
    
    QCOMPARE(pool.usedConnections(), 2);
    QCOMPARE(pool.availableConnections(), 0);
    
    // 归还第一个连接
    pool.returnConnection(conn1);
    QCOMPARE(pool.usedConnections(), 1);
    QCOMPARE(pool.availableConnections(), 1);
    
    // 归还第二个连接
    pool.returnConnection(conn2);
    QCOMPARE(pool.usedConnections(), 0);
    QCOMPARE(pool.availableConnections(), 2);
}

void TestConnectionPool::testConnectionPoolExhaustion()
{
    DatabaseConfig config = createTestConfig();
    config.maxConnections = 2;
    ConnectionPool pool(config);
    
    // 获取所有可用连接
    auto conn1 = pool.getConnection();
    auto conn2 = pool.getConnection();
    
    QCOMPARE(pool.usedConnections(), 2);
    
    // 尝试获取超出限制的连接应该抛出异常
    QVERIFY_EXCEPTION_THROWN(pool.getConnection(), ConnectionException);
}

void TestConnectionPool::testConnectionValidation()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    auto connection = pool.getConnection();
    QVERIFY(connection != nullptr);
    QVERIFY(connection->isOpen());
    QVERIFY(connection->isValid());
    
    pool.returnConnection(connection);
}

void TestConnectionPool::testConnectionPoolClose()
{
    DatabaseConfig config = createTestConfig();
    ConnectionPool pool(config);
    
    auto connection = pool.getConnection();
    QVERIFY(connection != nullptr);
    
    // 关闭连接池
    pool.close();
    
    // 关闭后应该无法获取新连接
    QVERIFY_EXCEPTION_THROWN(pool.getConnection(), ConnectionException);
    
    // 验证连接池状态
    QCOMPARE(pool.availableConnections(), 0);
    QCOMPARE(pool.usedConnections(), 0);
}

void TestConnectionPool::testIdleConnectionCleanup()
{
    DatabaseConfig config = createTestConfig();
    config.maxIdleTime = 1; // 1秒超时
    config.minConnections = 1;
    config.maxConnections = 5;
    
    ConnectionPool pool(config);
    
    // 获取并归还多个连接以创建空闲连接
    auto conn1 = pool.getConnection();
    auto conn2 = pool.getConnection();
    auto conn3 = pool.getConnection();
    
    pool.returnConnection(conn1);
    pool.returnConnection(conn2);
    pool.returnConnection(conn3);
    
    QCOMPARE(pool.availableConnections(), 4); // 1个初始 + 3个归还的
    
    // 等待超过空闲时间
    QTest::qWait(2000);
    
    // 手动触发清理（在实际使用中由定时器触发）
    QMetaObject::invokeMethod(&pool, "cleanupIdleConnections", Qt::DirectConnection);
    
    // 验证保持了最小连接数
    QVERIFY(pool.availableConnections() >= config.minConnections);
    QVERIFY(pool.availableConnections() < 4); // 应该清理了一些连接
}

void TestConnectionPool::testConcurrentAccess()
{
    DatabaseConfig config = createTestConfig();
    config.maxConnections = 10;
    config.minConnections = 2;
    
    ConnectionPool pool(config);
    
    const int threadCount = 5;
    const int operationsPerThread = 10;
    QVector<QSharedPointer<QSqlDatabase>> connections;
    QMutex connectionsMutex;
    
    // 创建多个线程同时获取和归还连接
    QThreadPool threadPool;
    QAtomicInt completedThreads(0);
    
    for (int i = 0; i < threadCount; ++i) {
        QRunnable* task = QRunnable::create([&pool, &connections, &connectionsMutex, operationsPerThread, &completedThreads]() {
            QVector<QSharedPointer<QSqlDatabase>> localConnections;
            
            // 获取连接
            for (int j = 0; j < operationsPerThread; ++j) {
                try {
                    auto conn = pool.getConnection();
                    if (conn) {
                        localConnections.append(conn);
                        // 模拟一些工作
                        QThread::msleep(10);
                    }
                } catch (const ConnectionException&) {
                    // 连接池耗尽是正常的
                }
            }
            
            // 归还连接
            for (auto conn : localConnections) {
                pool.returnConnection(conn);
            }
            
            completedThreads.fetchAndAddOrdered(1);
        });
        
        threadPool.start(task);
    }
    
    // 等待所有线程完成
    threadPool.waitForDone(10000); // 10秒超时
    
    QCOMPARE(completedThreads.loadAcquire(), threadCount);
    QCOMPARE(pool.usedConnections(), 0); // 所有连接都应该被归还
    QVERIFY(pool.availableConnections() >= config.minConnections);
}

void TestConnectionPool::testThreadSafety()
{
    DatabaseConfig config = createTestConfig();
    config.maxConnections = 3;
    config.minConnections = 1;
    
    ConnectionPool pool(config);
    
    QAtomicInt successfulGets(0);
    QAtomicInt successfulReturns(0);
    QAtomicInt exceptions(0);
    
    const int threadCount = 10;
    QThreadPool threadPool;
    QAtomicInt completedThreads(0);
    
    for (int i = 0; i < threadCount; ++i) {
        QRunnable* task = QRunnable::create([&pool, &successfulGets, &successfulReturns, &exceptions, &completedThreads]() {
            for (int j = 0; j < 5; ++j) {
                try {
                    auto conn = pool.getConnection();
                    if (conn) {
                        successfulGets.fetchAndAddOrdered(1);
                        
                        // 模拟一些数据库操作
                        QThread::msleep(50);
                        
                        pool.returnConnection(conn);
                        successfulReturns.fetchAndAddOrdered(1);
                    }
                } catch (const ConnectionException&) {
                    exceptions.fetchAndAddOrdered(1);
                }
            }
            completedThreads.fetchAndAddOrdered(1);
        });
        
        threadPool.start(task);
    }
    
    // 等待所有线程完成
    threadPool.waitForDone(15000); // 15秒超时
    
    QCOMPARE(completedThreads.loadAcquire(), threadCount);
    QCOMPARE(successfulGets.loadAcquire(), successfulReturns.loadAcquire());
    QCOMPARE(pool.usedConnections(), 0); // 所有连接都应该被归还
    
    // 验证连接池状态一致性
    QVERIFY(pool.totalConnections() >= config.minConnections);
    QVERIFY(pool.totalConnections() <= config.maxConnections);
}

#include "test_connectionpool.moc"