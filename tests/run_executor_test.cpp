#include <QtTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/datamodels.h"

using namespace QtMyBatisORM;

class TestExecutor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testConstructor();
    void testQuerySingle();
    void testQueryList();
    void testUpdate();
    void testInsert();
    void testDelete();
    void testQueryWithParameters();
    void testQueryWithCache();
    void testQueryListWithCache();
    void testInvalidConnection();
    void testSqlExecutionError();
    void testCacheKeyGeneration();

private:
    void setupTestDatabase();
    void createTestTable();
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<CacheManager> m_cacheManager;
    QSharedPointer<Executor> m_executor;
    QString m_connectionName;
};

void TestExecutor::initTestCase()
{
    // 设置测试数据库连接
    m_connectionName = "test_executor_connection";
    setupTestDatabase();
}

void TestExecutor::cleanupTestCase()
{
    if (m_connection && m_connection->isOpen()) {
        m_connection->close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void TestExecutor::init()
{
    // 为每个测试创建新的执行器实例
    if (m_connection && m_connection->isOpen()) {
        createTestTable();
        
        // 创建缓存管理器（使用默认配置）
        DatabaseConfig config;
        config.cacheEnabled = true;
        config.maxCacheSize = 100;
        config.cacheExpireTime = 300;
        m_cacheManager = QSharedPointer<CacheManager>::create(config);
        
        // 创建执行器
        m_executor = QSharedPointer<Executor>::create(m_connection, m_cacheManager);
    }
}

void TestExecutor::cleanup()
{
    // 清理测试数据
    if (m_connection && m_connection->isOpen()) {
        QSqlQuery query(*m_connection);
        query.exec("DROP TABLE IF EXISTS test_users");
    }
    
    m_executor.reset();
    m_cacheManager.reset();
}

void TestExecutor::setupTestDatabase()
{
    // 使用SQLite内存数据库进行测试
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        QFAIL("Failed to open test database");
    }
    
    m_connection = QSharedPointer<QSqlDatabase>::create(db);
}

void TestExecutor::createTestTable()
{
    QSqlQuery query(*m_connection);
    QString createTableSql = R"(
        CREATE TABLE test_users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE,
            age INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createTableSql)) {
        QFAIL(QString("Failed to create test table: %1").arg(query.lastError().text()).toLocal8Bit());
    }
    
    // 插入测试数据
    query.exec("INSERT INTO test_users (name, email, age) VALUES ('Alice', 'alice@example.com', 25)");
    query.exec("INSERT INTO test_users (name, email, age) VALUES ('Bob', 'bob@example.com', 30)");
    query.exec("INSERT INTO test_users (name, email, age) VALUES ('Charlie', 'charlie@example.com', 35)");
}

void TestExecutor::testConstructor()
{
    QVERIFY(m_executor != nullptr);
    QVERIFY(m_connection != nullptr);
    QVERIFY(m_cacheManager != nullptr);
}

void TestExecutor::testQuerySingle()
{
    QString sql = "SELECT * FROM test_users WHERE name = :name";
    QVariantMap parameters;
    parameters["name"] = "Alice";
    
    QVariant result = m_executor->query(sql, parameters);
    QVERIFY(!result.isNull());
    
    QVariantMap record = result.toMap();
    QCOMPARE(record["name"].toString(), QString("Alice"));
    QCOMPARE(record["email"].toString(), QString("alice@example.com"));
    QCOMPARE(record["age"].toInt(), 25);
}

void TestExecutor::testQueryList()
{
    QString sql = "SELECT * FROM test_users WHERE age >= :minAge ORDER BY age";
    QVariantMap parameters;
    parameters["minAge"] = 30;
    
    QVariantList results = m_executor->queryList(sql, parameters);
    QCOMPARE(results.size(), 2);
    
    QVariantMap firstRecord = results[0].toMap();
    QCOMPARE(firstRecord["name"].toString(), QString("Bob"));
    QCOMPARE(firstRecord["age"].toInt(), 30);
    
    QVariantMap secondRecord = results[1].toMap();
    QCOMPARE(secondRecord["name"].toString(), QString("Charlie"));
    QCOMPARE(secondRecord["age"].toInt(), 35);
}

void TestExecutor::testUpdate()
{
    QString sql = "UPDATE test_users SET age = :newAge WHERE name = :name";
    QVariantMap parameters;
    parameters["newAge"] = 26;
    parameters["name"] = "Alice";
    
    int affectedRows = m_executor->update(sql, parameters);
    QCOMPARE(affectedRows, 1);
    
    // 验证更新是否成功
    QString verifySql = "SELECT age FROM test_users WHERE name = :name";
    QVariantMap verifyParams;
    verifyParams["name"] = "Alice";
    
    QVariant result = m_executor->query(verifySql, verifyParams);
    QVariantMap record = result.toMap();
    QCOMPARE(record["age"].toInt(), 26);
}

void TestExecutor::testInsert()
{
    QString sql = "INSERT INTO test_users (name, email, age) VALUES (:name, :email, :age)";
    QVariantMap parameters;
    parameters["name"] = "David";
    parameters["email"] = "david@example.com";
    parameters["age"] = 28;
    
    int affectedRows = m_executor->update(sql, parameters);
    QCOMPARE(affectedRows, 1);
    
    // 验证插入是否成功
    QString verifySql = "SELECT COUNT(*) as count FROM test_users WHERE name = :name";
    QVariantMap verifyParams;
    verifyParams["name"] = "David";
    
    QVariant result = m_executor->query(verifySql, verifyParams);
    QVariantMap record = result.toMap();
    QCOMPARE(record["count"].toInt(), 1);
}

void TestExecutor::testDelete()
{
    QString sql = "DELETE FROM test_users WHERE name = :name";
    QVariantMap parameters;
    parameters["name"] = "Charlie";
    
    int affectedRows = m_executor->update(sql, parameters);
    QCOMPARE(affectedRows, 1);
    
    // 验证删除是否成功
    QString verifySql = "SELECT COUNT(*) as count FROM test_users WHERE name = :name";
    QVariantMap verifyParams;
    verifyParams["name"] = "Charlie";
    
    QVariant result = m_executor->query(verifySql, verifyParams);
    QVariantMap record = result.toMap();
    QCOMPARE(record["count"].toInt(), 0);
}

void TestExecutor::testQueryWithParameters()
{
    // 测试多个参数
    QString sql = "SELECT * FROM test_users WHERE age BETWEEN :minAge AND :maxAge ORDER BY age";
    QVariantMap parameters;
    parameters["minAge"] = 25;
    parameters["maxAge"] = 30;
    
    QVariantList results = m_executor->queryList(sql, parameters);
    QCOMPARE(results.size(), 2);
    
    // 测试空参数
    QString sql2 = "SELECT COUNT(*) as total FROM test_users";
    QVariantMap emptyParams;
    
    QVariant result = m_executor->query(sql2, emptyParams);
    QVariantMap record = result.toMap();
    QCOMPARE(record["total"].toInt(), 3);
}

void TestExecutor::testQueryWithCache()
{
    QString statementId = "getUserByName";
    QString sql = "SELECT * FROM test_users WHERE name = :name";
    QVariantMap parameters;
    parameters["name"] = "Alice";
    
    // 第一次查询，应该从数据库获取
    QVariant result1 = m_executor->queryWithCache(statementId, sql, parameters);
    QVERIFY(!result1.isNull());
    
    // 第二次查询，应该从缓存获取
    QVariant result2 = m_executor->queryWithCache(statementId, sql, parameters);
    QVERIFY(!result2.isNull());
    
    // 结果应该相同
    QVariantMap record1 = result1.toMap();
    QVariantMap record2 = result2.toMap();
    QCOMPARE(record1["name"].toString(), record2["name"].toString());
    QCOMPARE(record1["email"].toString(), record2["email"].toString());
}

void TestExecutor::testQueryListWithCache()
{
    QString statementId = "getUsersByAge";
    QString sql = "SELECT * FROM test_users WHERE age >= :minAge ORDER BY age";
    QVariantMap parameters;
    parameters["minAge"] = 25;
    
    // 第一次查询，应该从数据库获取
    QVariantList result1 = m_executor->queryListWithCache(statementId, sql, parameters);
    QCOMPARE(result1.size(), 3);
    
    // 第二次查询，应该从缓存获取
    QVariantList result2 = m_executor->queryListWithCache(statementId, sql, parameters);
    QCOMPARE(result2.size(), 3);
    
    // 结果应该相同
    QVariantMap firstRecord1 = result1[0].toMap();
    QVariantMap firstRecord2 = result2[0].toMap();
    QCOMPARE(firstRecord1["name"].toString(), firstRecord2["name"].toString());
}

void TestExecutor::testInvalidConnection()
{
    // 创建一个无效连接的执行器
    QSharedPointer<QSqlDatabase> invalidConnection;
    QSharedPointer<Executor> invalidExecutor = QSharedPointer<Executor>::create(invalidConnection, m_cacheManager);
    
    QString sql = "SELECT * FROM test_users";
    QVariantMap parameters;
    
    // 应该抛出连接异常
    QVERIFY_EXCEPTION_THROWN(invalidExecutor->query(sql, parameters), ConnectionException);
    QVERIFY_EXCEPTION_THROWN(invalidExecutor->queryList(sql, parameters), ConnectionException);
    QVERIFY_EXCEPTION_THROWN(invalidExecutor->update(sql, parameters), ConnectionException);
}

void TestExecutor::testSqlExecutionError()
{
    // 测试无效的SQL语句
    QString invalidSql = "SELECT * FROM non_existent_table";
    QVariantMap parameters;
    
    QVERIFY_EXCEPTION_THROWN(m_executor->query(invalidSql, parameters), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_executor->queryList(invalidSql, parameters), SqlExecutionException);
    QVERIFY_EXCEPTION_THROWN(m_executor->update(invalidSql, parameters), SqlExecutionException);
}

void TestExecutor::testCacheKeyGeneration()
{
    // 测试缓存键生成的一致性
    QString statementId = "testStatement";
    QVariantMap parameters1;
    parameters1["name"] = "Alice";
    parameters1["age"] = 25;
    
    QVariantMap parameters2;
    parameters2["age"] = 25;
    parameters2["name"] = "Alice";  // 相同参数，不同顺序
    
    // 由于我们无法直接访问generateCacheKey方法，我们通过缓存行为来测试
    QString sql = "SELECT * FROM test_users WHERE name = :name AND age = :age";
    
    // 使用相同参数的查询应该产生相同的缓存键
    QVariant result1 = m_executor->queryWithCache(statementId, sql, parameters1);
    QVariant result2 = m_executor->queryWithCache(statementId, sql, parameters2);
    
    // 两次查询应该返回相同的结果（第二次从缓存获取）
    QVERIFY(!result1.isNull());
    QVERIFY(!result2.isNull());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestExecutor test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_executor_test.moc"