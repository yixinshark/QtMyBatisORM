#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include <QThread>
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestSessionTransaction : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 基本事务测试
    void testBasicTransactionCommit();
    void testBasicTransactionRollback();
    void testTransactionWithoutBegin();
    void testDoubleBeginTransaction();
    
    // 事务超时测试
    void testTransactionTimeout();
    void testTransactionTimeoutCheck();
    void testTransactionWithoutTimeout();
    
    // 嵌套事务测试 (保存点)
    void testSavepointCreation();
    void testSavepointRollback();
    void testSavepointRelease();
    void testMultipleSavepoints();
    void testSavepointOutsideTransaction();
    
    // 事务状态查询测试
    void testTransactionLevel();
    void testTransactionStartTime();
    void testTransactionTimeoutStatus();
    
    // 错误处理测试
    void testTransactionWithClosedSession();
    void testTransactionWithInvalidConnection();
    void testSavepointErrors();

private:
    void setupTestDatabase();
    void createTestTable();
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<Executor> m_executor;
    QSharedPointer<MapperRegistry> m_mapperRegistry;
    QSharedPointer<Session> m_session;
    QString m_connectionName;
};

void TestSessionTransaction::initTestCase()
{
    // 注册SQLite驱动
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QSKIP("SQLite driver not available");
    }
}

void TestSessionTransaction::cleanupTestCase()
{
    if (m_connection && m_connection->isOpen()) {
        m_connection->close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void TestSessionTransaction::init()
{
    setupTestDatabase();
    createTestTable();
    
    DatabaseConfig config;
    config.driverName = "QSQLITE";
    config.databaseName = ":memory:";
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 300;
    
    auto cacheManager = QSharedPointer<CacheManager>::create(config);
    m_executor = QSharedPointer<Executor>::create(m_connection, cacheManager);
    m_mapperRegistry = QSharedPointer<MapperRegistry>::create();
    m_session = QSharedPointer<Session>::create(m_connection, m_executor, m_mapperRegistry);
}

void TestSessionTransaction::cleanup()
{
    if (m_session) {
        if (m_session->isInTransaction()) {
            m_session->rollback();
        }
        m_session->close();
        m_session.reset();
    }
    
    if (m_connection && m_connection->isOpen()) {
        m_connection->close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void TestSessionTransaction::setupTestDatabase()
{
    m_connectionName = QString("test_session_transaction_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        QFAIL(QString("Failed to open database: %1").arg(db.lastError().text()).toLocal8Bit());
    }
    
    m_connection = QSharedPointer<QSqlDatabase>::create(db);
}

void TestSessionTransaction::createTestTable()
{
    QSqlQuery query(*m_connection);
    QString sql = "CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)";
    
    if (!query.exec(sql)) {
        QFAIL(QString("Failed to create test table: %1").arg(query.lastError().text()).toLocal8Bit());
    }
}

// 基本事务测试
void TestSessionTransaction::testBasicTransactionCommit()
{
    QVERIFY(!m_session->isInTransaction());
    
    // 开始事务
    m_session->beginTransaction();
    QVERIFY(m_session->isInTransaction());
    QCOMPARE(m_session->getTransactionLevel(), 1);
    
    // 插入数据
    QSqlQuery query(*m_connection);
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('test1', 100)"));
    
    // 提交事务
    m_session->commit();
    QVERIFY(!m_session->isInTransaction());
    QCOMPARE(m_session->getTransactionLevel(), 0);
    
    // 验证数据已提交
    QVERIFY(query.exec("SELECT COUNT(*) FROM test_table"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void TestSessionTransaction::testBasicTransactionRollback()
{
    QVERIFY(!m_session->isInTransaction());
    
    // 开始事务
    m_session->beginTransaction();
    QVERIFY(m_session->isInTransaction());
    
    // 插入数据
    QSqlQuery query(*m_connection);
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('test1', 100)"));
    
    // 回滚事务
    m_session->rollback();
    QVERIFY(!m_session->isInTransaction());
    QCOMPARE(m_session->getTransactionLevel(), 0);
    
    // 验证数据已回滚
    QVERIFY(query.exec("SELECT COUNT(*) FROM test_table"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void TestSessionTransaction::testTransactionWithoutBegin()
{
    QVERIFY(!m_session->isInTransaction());
    
    // 尝试提交没有开始的事务
    QVERIFY_EXCEPTION_THROWN(m_session->commit(), SqlExecutionException);
    
    // 回滚没有开始的事务应该静默成功
    m_session->rollback(); // 不应该抛出异常
    QVERIFY(!m_session->isInTransaction());
}

void TestSessionTransaction::testDoubleBeginTransaction()
{
    m_session->beginTransaction();
    QVERIFY(m_session->isInTransaction());
    
    // 尝试在已有事务中开始新事务
    QVERIFY_EXCEPTION_THROWN(m_session->beginTransaction(), SqlExecutionException);
    
    // 原事务应该仍然活跃
    QVERIFY(m_session->isInTransaction());
}

// 事务超时测试
void TestSessionTransaction::testTransactionTimeout()
{
    // 开始一个1秒超时的事务
    m_session->beginTransaction(1);
    QVERIFY(m_session->isInTransaction());
    QVERIFY(!m_session->isTransactionTimedOut());
    
    // 等待超时
    QTest::qWait(1100); // 等待1.1秒
    
    // 检查超时状态
    QVERIFY(m_session->isTransactionTimedOut());
    
    // 任何操作都应该触发超时检查并回滚事务
    QVERIFY_EXCEPTION_THROWN(m_session->commit(), SqlExecutionException);
    QVERIFY(!m_session->isInTransaction());
}

void TestSessionTransaction::testTransactionTimeoutCheck()
{
    QDateTime startTime = QDateTime::currentDateTime();
    m_session->beginTransaction(2);
    
    // 验证事务开始时间
    QDateTime transactionStart = m_session->getTransactionStartTime();
    QVERIFY(transactionStart >= startTime);
    QVERIFY(transactionStart <= QDateTime::currentDateTime());
    
    // 在超时前应该正常工作
    QVERIFY(!m_session->isTransactionTimedOut());
    
    // 提交应该成功
    m_session->commit();
    QVERIFY(!m_session->isInTransaction());
}

void TestSessionTransaction::testTransactionWithoutTimeout()
{
    // 开始无超时事务
    m_session->beginTransaction(); // 默认无超时
    QVERIFY(m_session->isInTransaction());
    QVERIFY(!m_session->isTransactionTimedOut());
    
    // 等待一段时间
    QTest::qWait(100);
    
    // 仍然不应该超时
    QVERIFY(!m_session->isTransactionTimedOut());
    
    m_session->commit();
    QVERIFY(!m_session->isInTransaction());
}

// 嵌套事务测试 (保存点)
void TestSessionTransaction::testSavepointCreation()
{
    m_session->beginTransaction();
    QCOMPARE(m_session->getTransactionLevel(), 1);
    
    // 创建保存点
    QString sp1 = m_session->setSavepoint();
    QVERIFY(!sp1.isEmpty());
    QCOMPARE(m_session->getTransactionLevel(), 2);
    
    // 创建命名保存点
    QString sp2 = m_session->setSavepoint("my_savepoint");
    QCOMPARE(sp2, QString("my_savepoint"));
    QCOMPARE(m_session->getTransactionLevel(), 3);
    
    m_session->commit();
}

void TestSessionTransaction::testSavepointRollback()
{
    m_session->beginTransaction();
    
    // 插入第一条记录
    QSqlQuery query(*m_connection);
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('record1', 1)"));
    
    // 创建保存点
    QString sp1 = m_session->setSavepoint("sp1");
    
    // 插入第二条记录
    QVERIFY(query.exec("INSERT INTO test_table (name, value) VALUES ('record2', 2)"));
    
    // 回滚到保存点
    m_session->rollbackToSavepoint(sp1);
    
    // 提交事务
    m_session->commit();
    
    // 验证只有第一条记录存在
    QVERIFY(query.exec("SELECT COUNT(*) FROM test_table"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    
    QVERIFY(query.exec("SELECT name FROM test_table"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QString("record1"));
}

void TestSessionTransaction::testSavepointRelease()
{
    m_session->beginTransaction();
    
    QString sp1 = m_session->setSavepoint("sp1");
    QString sp2 = m_session->setSavepoint("sp2");
    
    QCOMPARE(m_session->getTransactionLevel(), 3);
    
    // 释放保存点
    m_session->releaseSavepoint(sp1);
    QCOMPARE(m_session->getTransactionLevel(), 2);
    
    m_session->commit();
}

void TestSessionTransaction::testMultipleSavepoints()
{
    m_session->beginTransaction();
    
    QString sp1 = m_session->setSavepoint("sp1");
    QString sp2 = m_session->setSavepoint("sp2");
    QString sp3 = m_session->setSavepoint("sp3");
    
    QCOMPARE(m_session->getTransactionLevel(), 4);
    
    // 回滚到中间的保存点应该移除后续的保存点
    m_session->rollbackToSavepoint(sp2);
    QCOMPARE(m_session->getTransactionLevel(), 3); // sp3被移除
    
    m_session->commit();
}

void TestSessionTransaction::testSavepointOutsideTransaction()
{
    QVERIFY(!m_session->isInTransaction());
    
    // 在事务外创建保存点应该失败
    QVERIFY_EXCEPTION_THROWN(m_session->setSavepoint(), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_session->rollbackToSavepoint("sp1"), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_session->releaseSavepoint("sp1"), SqlExecutionException);
}

// 事务状态查询测试
void TestSessionTransaction::testTransactionLevel()
{
    QCOMPARE(m_session->getTransactionLevel(), 0);
    
    m_session->beginTransaction();
    QCOMPARE(m_session->getTransactionLevel(), 1);
    
    QString sp1 = m_session->setSavepoint();
    QCOMPARE(m_session->getTransactionLevel(), 2);
    
    QString sp2 = m_session->setSavepoint();
    QCOMPARE(m_session->getTransactionLevel(), 3);
    
    m_session->rollbackToSavepoint(sp1);
    QCOMPARE(m_session->getTransactionLevel(), 2);
    
    m_session->commit();
    QCOMPARE(m_session->getTransactionLevel(), 0);
}

void TestSessionTransaction::testTransactionStartTime()
{
    QDateTime beforeStart = QDateTime::currentDateTime();
    m_session->beginTransaction();
    QDateTime afterStart = QDateTime::currentDateTime();
    
    QDateTime startTime = m_session->getTransactionStartTime();
    QVERIFY(startTime >= beforeStart);
    QVERIFY(startTime <= afterStart);
    
    m_session->commit();
    
    // 事务结束后开始时间应该被清除
    QVERIFY(!m_session->getTransactionStartTime().isValid());
}

void TestSessionTransaction::testTransactionTimeoutStatus()
{
    // 无超时事务
    m_session->beginTransaction();
    QVERIFY(!m_session->isTransactionTimedOut());
    m_session->commit();
    
    // 有超时但未超时的事务
    m_session->beginTransaction(5);
    QVERIFY(!m_session->isTransactionTimedOut());
    m_session->commit();
}

// 错误处理测试
void TestSessionTransaction::testTransactionWithClosedSession()
{
    m_session->close();
    QVERIFY(m_session->isClosed());
    
    // 在关闭的会话上进行事务操作应该失败
    QVERIFY_EXCEPTION_THROWN(m_session->beginTransaction(), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_session->commit(), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_session->setSavepoint(), SqlExecutionException);
}

void TestSessionTransaction::testTransactionWithInvalidConnection()
{
    // 关闭数据库连接
    m_connection->close();
    
    // 尝试开始事务应该失败
    QVERIFY_EXCEPTION_THROWN(m_session->beginTransaction(), SqlExecutionException);
}

void TestSessionTransaction::testSavepointErrors()
{
    m_session->beginTransaction();
    
    // 回滚到不存在的保存点
    QVERIFY_EXCEPTION_THROWN(m_session->rollbackToSavepoint("nonexistent"), SqlExecutionException);
    
    // 释放不存在的保存点
    QVERIFY_EXCEPTION_THROWN(m_session->releaseSavepoint("nonexistent"), SqlExecutionException);
    
    m_session->commit();
}

QTEST_MAIN(TestSessionTransaction)

#include "test_session_transaction.moc"