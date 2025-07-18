#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/configurationmanager.h"

using namespace QtMyBatisORM;

class TestSessionFactory : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testCreateSessionFactory();
    void testCreateSessionFactoryWithInvalidConfig();
    void testOpenSession();
    void testOpenSessionWhenClosed();
    void testCloseSession();
    void testSessionLifecycleManagement();
    void testMultipleSessions();
    void testGetMapper();
    void testCloseFactory();
    void testActiveSessionCount();

private:
    DatabaseConfig createTestConfig();
    void setupTestDatabase();
    
    QTemporaryDir* m_tempDir;
    QString m_dbPath;
};

void TestSessionFactory::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->path() + "/test.db";
    
    setupTestDatabase();
}

void TestSessionFactory::cleanupTestCase()
{
    // 清理数据库连接
    QSqlDatabase::removeDatabase("QSQLITE");
    delete m_tempDir;
}

void TestSessionFactory::init()
{
    // 每个测试前的初始化
}

void TestSessionFactory::cleanup()
{
    // 每个测试后的清理
}

DatabaseConfig TestSessionFactory::createTestConfig()
{
    DatabaseConfig config;
    config.driverName = "QSQLITE";
    config.databaseName = m_dbPath;
    config.maxConnections = 5;
    config.minConnections = 1;
    config.maxIdleTime = 300;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 600;
    return config;
}

void TestSessionFactory::setupTestDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(m_dbPath);
    
    QVERIFY(db.open());
    
    QSqlQuery query(db);
    QVERIFY(query.exec("CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"));
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('test1', 100)"));
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('test2', 200)"));
    
    db.close();
}

void TestSessionFactory::testCreateSessionFactory()
{
    DatabaseConfig config = createTestConfig();
    
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QVERIFY(!factory.isNull());
    QVERIFY(!factory->isClosed());
    QCOMPARE(factory->getActiveSessionCount(), 0);
    
    factory->close();
}

void TestSessionFactory::testCreateSessionFactoryWithInvalidConfig()
{
    DatabaseConfig config;
    config.driverName = "INVALID_DRIVER";
    config.databaseName = "invalid_db";
    
    // 创建SessionFactory应该成功，但在尝试获取连接时会失败
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    QVERIFY(!factory.isNull());
    
    factory->close();
}

void TestSessionFactory::testOpenSession()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QSharedPointer<Session> session = factory->openSession();
    
    QVERIFY(!session.isNull());
    QVERIFY(!session->isClosed());
    QCOMPARE(factory->getActiveSessionCount(), 1);
    
    factory->closeSession(session);
    factory->close();
}

void TestSessionFactory::testOpenSessionWhenClosed()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    factory->close();
    
    QVERIFY_EXCEPTION_THROWN(factory->openSession(), ConfigurationException);
}

void TestSessionFactory::testCloseSession()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QSharedPointer<Session> session = factory->openSession();
    QCOMPARE(factory->getActiveSessionCount(), 1);
    
    factory->closeSession(session);
    QCOMPARE(factory->getActiveSessionCount(), 0);
    QVERIFY(session->isClosed());
    
    factory->close();
}

void TestSessionFactory::testSessionLifecycleManagement()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    {
        QSharedPointer<Session> session = factory->openSession();
        QCOMPARE(factory->getActiveSessionCount(), 1);
        
        // Session应该在作用域结束时自动清理
    }
    
    // 给Qt事件循环一些时间来处理对象销毁
    QTest::qWait(10);
    
    // 活动Session数量应该减少到0
    QCOMPARE(factory->getActiveSessionCount(), 0);
    
    factory->close();
}

void TestSessionFactory::testMultipleSessions()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QList<QSharedPointer<Session>> sessions;
    
    // 创建多个Session
    for (int i = 0; i < 3; ++i) {
        QSharedPointer<Session> session = factory->openSession();
        QVERIFY(!session.isNull());
        sessions.append(session);
    }
    
    QCOMPARE(factory->getActiveSessionCount(), 3);
    
    // 关闭所有Session
    for (auto& session : sessions) {
        factory->closeSession(session);
    }
    
    QCOMPARE(factory->getActiveSessionCount(), 0);
    
    factory->close();
}

void TestSessionFactory::testGetMapper()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QSharedPointer<Session> session = factory->openSession();
    
    // 由于我们还没有实现具体的Mapper类，这里只测试方法调用不会崩溃
    // 实际的Mapper测试将在Mapper实现完成后进行
    
    factory->closeSession(session);
    factory->close();
}

void TestSessionFactory::testCloseFactory()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    // 创建一些Session
    QSharedPointer<Session> session1 = factory->openSession();
    QSharedPointer<Session> session2 = factory->openSession();
    
    QCOMPARE(factory->getActiveSessionCount(), 2);
    
    // 关闭工厂应该关闭所有活动的Session
    factory->close();
    
    QVERIFY(factory->isClosed());
    QCOMPARE(factory->getActiveSessionCount(), 0);
    QVERIFY(session1->isClosed());
    QVERIFY(session2->isClosed());
}

void TestSessionFactory::testActiveSessionCount()
{
    DatabaseConfig config = createTestConfig();
    QSharedPointer<SessionFactory> factory = SessionFactory::create(config);
    
    QCOMPARE(factory->getActiveSessionCount(), 0);
    
    QSharedPointer<Session> session1 = factory->openSession();
    QCOMPARE(factory->getActiveSessionCount(), 1);
    
    QSharedPointer<Session> session2 = factory->openSession();
    QCOMPARE(factory->getActiveSessionCount(), 2);
    
    factory->closeSession(session1);
    QCOMPARE(factory->getActiveSessionCount(), 1);
    
    factory->closeSession(session2);
    QCOMPARE(factory->getActiveSessionCount(), 0);
    
    factory->close();
}

#include "test_sessionfactory.moc"