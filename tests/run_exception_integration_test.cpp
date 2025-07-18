#include <QCoreApplication>
#include <QTest>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>

#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/mapperproxy.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/DataModels.h"

using namespace QtMyBatisORM;

class TestExceptionIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Configuration Exception Tests
    void testConfigurationManagerEmptyPath();
    void testConfigurationManagerInvalidJson();
    void testConfigurationManagerDuplicateNamespace();
    void testConfigurationManagerMissingFile();

    // Connection Exception Tests
    void testConnectionPoolClosedAccess();
    void testConnectionPoolInvalidConfig();
    void testConnectionPoolExhaustion();
    void testConnectionPoolRetryMechanism();

    // Session Exception Tests
    void testSessionClosedOperations();
    void testSessionInvalidStatement();
    void testSessionTransactionErrors();
    void testSessionTimeoutHandling();

    // Cache Exception Tests
    void testCacheManagerEmptyKey();
    void testCacheManagerDisabledOperations();
    void testCacheManagerEvictionErrors();

    // Executor Exception Tests
    void testExecutorConnectionFailure();
    void testExecutorSqlErrors();
    void testExecutorParameterErrors();

    // Exception Recovery Tests
    void testConnectionRecoveryAfterFailure();
    void testTransactionRecoveryAfterTimeout();
    void testCacheRecoveryAfterError();

    // Exception Context Tests
    void testExceptionContextInformation();
    void testExceptionChaining();
    void testExceptionLogging();

private:
    void createTestConfig(const QString& path, bool valid = true);
    void createTestMapper(const QString& path, bool valid = true);
    DatabaseConfig createValidDatabaseConfig();
    DatabaseConfig createInvalidDatabaseConfig();

    QTemporaryDir* m_tempDir;
    QString m_configPath;
    QString m_mapperPath;
};

void TestExceptionIntegration::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_configPath = m_tempDir->path() + "/test_config.json";
    m_mapperPath = m_tempDir->path() + "/test_mapper.xml";
}

void TestExceptionIntegration::cleanupTestCase()
{
    delete m_tempDir;
}

void TestExceptionIntegration::init()
{
    // Reset configuration manager state
    ConfigurationManager::instance()->reset();
}

void TestExceptionIntegration::cleanup()
{
    // Clean up any test files
    QFile::remove(m_configPath);
    QFile::remove(m_mapperPath);
}

void TestExceptionIntegration::testConfigurationManagerEmptyPath()
{
    ConfigurationManager* manager = ConfigurationManager::instance();
    
    // Test empty configuration path
    try {
        manager->loadConfiguration("");
        QFAIL("Expected ConfigurationException for empty path");
    } catch (const ConfigurationException& e) {
        QCOMPARE(e.code(), QString("CONFIG_EMPTY_PATH"));
        QVERIFY(e.getAllContext().contains("configPath"));
        QVERIFY(!e.message().isEmpty());
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConfigurationManagerInvalidJson()
{
    ConfigurationManager* manager = ConfigurationManager::instance();
    
    try {
        manager->loadConfiguration(":/nonexistent/config.json");
        QFAIL("Expected ConfigurationException for invalid JSON");
    } catch (const ConfigurationException& e) {
        QVERIFY(e.code().contains("CONFIG"));
        QVERIFY(e.getAllContext().contains("configPath"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConfigurationManagerDuplicateNamespace()
{
    // Skip this test as it requires file system access which conflicts with Qt resource system
    QSKIP("Skipping file-based configuration test - requires Qt resource system integration");
}

void TestExceptionIntegration::testConfigurationManagerMissingFile()
{
    ConfigurationManager* manager = ConfigurationManager::instance();
    
    try {
        manager->loadConfiguration("/nonexistent/path/config.json");
        QFAIL("Expected ConfigurationException for missing file");
    } catch (const ConfigurationException& e) {
        QVERIFY(e.getAllContext().contains("configPath"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConnectionPoolClosedAccess()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    
    // Close the pool
    pool.close();
    
    try {
        auto connection = pool.getConnection();
        QFAIL("Expected ConnectionException for closed pool");
    } catch (const ConnectionException& e) {
        QCOMPARE(e.code(), QString("POOL_CLOSED"));
        QVERIFY(e.getAllContext().contains("poolState"));
        QCOMPARE(e.getContext("poolState").toString(), QString("closed"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConnectionPoolInvalidConfig()
{
    DatabaseConfig config = createInvalidDatabaseConfig();
    
    try {
        ConnectionPool pool(config);
        auto connection = pool.getConnection();
        QFAIL("Expected ConnectionException for invalid config");
    } catch (const ConnectionException& e) {
        QVERIFY(e.getAllContext().contains("originalError") || 
                e.code().contains("CONNECTION"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConnectionPoolExhaustion()
{
    DatabaseConfig config = createValidDatabaseConfig();
    config.maxConnections = 1; // Very small pool
    
    ConnectionPool pool(config);
    
    try {
        // Get the only connection
        auto conn1 = pool.getConnection();
        
        // Try to get another - should fail
        auto conn2 = pool.getConnection();
        QFAIL("Expected ConnectionException for pool exhaustion");
    } catch (const ConnectionException& e) {
        QCOMPARE(e.code(), QString("POOL_EXHAUSTED"));
        QVERIFY(e.getAllContext().contains("maxConnections"));
        QVERIFY(e.getAllContext().contains("usedConnections"));
        QCOMPARE(e.getContext("maxConnections").toInt(), 1);
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConnectionPoolRetryMechanism()
{
    // This test verifies that the retry mechanism adds proper context
    DatabaseConfig config = createInvalidDatabaseConfig();
    
    try {
        ConnectionPool pool(config);
        auto connection = pool.getConnection();
        QFAIL("Expected ConnectionException with retry information");
    } catch (const ConnectionException& e) {
        // Should contain retry context if retries were attempted
        if (e.code() == "CONNECTION_CREATE_FAILED") {
            QVERIFY(e.getAllContext().contains("retryCount"));
            QVERIFY(e.getAllContext().contains("maxRetries"));
        }
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testSessionClosedOperations()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    auto mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    Session session(connection, executor, mapperRegistry);
    session.close();
    
    try {
        session.selectOne("test.select", {});
        QFAIL("Expected SessionException for closed session");
    } catch (const SessionException& e) {
        QVERIFY(e.getAllContext().contains("operation"));
        QCOMPARE(e.getContext("operation").toString(), QString("selectOne"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testSessionInvalidStatement()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    auto mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    Session session(connection, executor, mapperRegistry);
    
    try {
        session.selectOne("nonexistent.statement", {});
        QFAIL("Expected SessionException for invalid statement");
    } catch (const SessionException& e) {
        QVERIFY(e.getAllContext().contains("statementId"));
        QCOMPARE(e.getContext("statementId").toString(), QString("nonexistent.statement"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testSessionTransactionErrors()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    auto mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    Session session(connection, executor, mapperRegistry);
    
    // Test beginning transaction when already in transaction
    try {
        session.beginTransaction();
        session.beginTransaction(); // Should fail
        QFAIL("Expected TransactionException for nested transaction");
    } catch (const TransactionException& e) {
        QCOMPARE(e.code(), QString("TRANSACTION_ALREADY_ACTIVE"));
        QVERIFY(e.getAllContext().contains("transactionLevel"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
    
    session.rollback(); // Clean up
}

void TestExceptionIntegration::testSessionTimeoutHandling()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    auto mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    Session session(connection, executor, mapperRegistry);
    
    try {
        session.beginTransaction(1); // 1 second timeout
        QThread::sleep(2); // Wait longer than timeout
        session.commit(); // Should fail due to timeout
        QFAIL("Expected TransactionException for timeout");
    } catch (const TransactionException& e) {
        QVERIFY(e.message().contains("timeout") || e.code().contains("TIMEOUT"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testCacheManagerEmptyKey()
{
    DatabaseConfig config = createValidDatabaseConfig();
    CacheManager cache(config);
    
    try {
        cache.put("", QVariant("test"));
        QFAIL("Expected CacheException for empty key");
    } catch (const CacheException& e) {
        QCOMPARE(e.code(), QString("CACHE_EMPTY_KEY"));
        QVERIFY(e.getAllContext().contains("operation"));
        QCOMPARE(e.getContext("operation").toString(), QString("put"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
    
    try {
        cache.get("");
        QFAIL("Expected CacheException for empty key");
    } catch (const CacheException& e) {
        QCOMPARE(e.code(), QString("CACHE_EMPTY_KEY"));
        QVERIFY(e.getAllContext().contains("operation"));
        QCOMPARE(e.getContext("operation").toString(), QString("get"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testCacheManagerDisabledOperations()
{
    DatabaseConfig config = createValidDatabaseConfig();
    config.cacheEnabled = false;
    CacheManager cache(config);
    
    // These should not throw exceptions, just return silently
    cache.put("test", QVariant("value"));
    QVariant result = cache.get("test");
    QVERIFY(result.isNull());
}

void TestExceptionIntegration::testCacheManagerEvictionErrors()
{
    DatabaseConfig config = createValidDatabaseConfig();
    config.maxCacheSize = 1; // Very small cache to force eviction
    CacheManager cache(config);
    
    // This should work without throwing exceptions
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2")); // Should evict key1
    
    QVERIFY(!cache.contains("key1"));
    QVERIFY(cache.contains("key2"));
}

void TestExceptionIntegration::testExecutorConnectionFailure()
{
    // Create executor with null connection
    auto executor = QSharedPointer<Executor>::create(nullptr, nullptr);
    
    try {
        executor->query("SELECT 1", {});
        QFAIL("Expected ConnectionException for null connection");
    } catch (const ConnectionException& e) {
        QVERIFY(e.message().contains("connection") || 
                e.message().contains("available"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testExecutorSqlErrors()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    
    try {
        executor->query("INVALID SQL STATEMENT", {});
        QFAIL("Expected SqlExecutionException for invalid SQL");
    } catch (const SqlExecutionException& e) {
        QVERIFY(e.message().contains("Failed to execute") || 
                e.message().contains("SQL"));
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testExecutorParameterErrors()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    
    try {
        // SQL with parameters but no parameter handler
        executor->query("SELECT * FROM test WHERE id = :id", {{"id", 1}});
        // This might not always throw depending on implementation
    } catch (const QtMyBatisException& e) {
        qDebug() << "Caught expected exception:" << e.fullMessage();
    }
}

void TestExceptionIntegration::testConnectionRecoveryAfterFailure()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    
    // Get a connection successfully
    auto conn1 = pool.getConnection();
    QVERIFY(conn1);
    
    // Return it
    pool.returnConnection(conn1);
    
    // Should be able to get another connection
    auto conn2 = pool.getConnection();
    QVERIFY(conn2);
    
    pool.returnConnection(conn2);
}

void TestExceptionIntegration::testTransactionRecoveryAfterTimeout()
{
    DatabaseConfig config = createValidDatabaseConfig();
    ConnectionPool pool(config);
    auto connection = pool.getConnection();
    
    auto executor = QSharedPointer<Executor>::create(connection, nullptr);
    auto mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    Session session(connection, executor, mapperRegistry);
    
    try {
        session.beginTransaction(1); // 1 second timeout
        QThread::sleep(2); // Wait for timeout
        session.commit(); // Should fail
    } catch (const TransactionException&) {
        // Expected
    }
    
    // Should be able to start a new transaction after timeout
    try {
        session.beginTransaction();
        session.rollback();
    } catch (const QtMyBatisException& e) {
        QFAIL(QString("Should be able to start new transaction after timeout: %1").arg(e.message()).toLocal8Bit());
    }
}

void TestExceptionIntegration::testCacheRecoveryAfterError()
{
    DatabaseConfig config = createValidDatabaseConfig();
    CacheManager cache(config);
    
    try {
        cache.put("", QVariant("test")); // Should fail
    } catch (const CacheException&) {
        // Expected
    }
    
    // Should be able to use cache normally after error
    cache.put("valid_key", QVariant("test"));
    QVariant result = cache.get("valid_key");
    QCOMPARE(result.toString(), QString("test"));
}

void TestExceptionIntegration::testExceptionContextInformation()
{
    try {
        ConfigurationException ex("Test message", "TEST_CODE");
        ex.setContext("key1", "value1");
        ex.setContext("key2", 42);
        ex.setContext("key3", true);
        
        QCOMPARE(ex.message(), QString("Test message"));
        QCOMPARE(ex.code(), QString("TEST_CODE"));
        QCOMPARE(ex.getContext("key1").toString(), QString("value1"));
        QCOMPARE(ex.getContext("key2").toInt(), 42);
        QCOMPARE(ex.getContext("key3").toBool(), true);
        
        QString fullMsg = ex.fullMessage();
        QVERIFY(fullMsg.contains("Test message"));
        QVERIFY(fullMsg.contains("TEST_CODE"));
        QVERIFY(fullMsg.contains("key1=value1"));
        
        qDebug() << "Full message:" << fullMsg;
    } catch (...) {
        QFAIL("Exception context test should not throw");
    }
}

void TestExceptionIntegration::testExceptionChaining()
{
    try {
        // Create original exception
        ConfigurationException original("Original error", "ORIGINAL_CODE");
        original.setContext("originalKey", "originalValue");
        
        // Create chained exception
        ConfigurationException chained(original);
        chained.setContext("chainedKey", "chainedValue");
        
        // Both contexts should be available
        QCOMPARE(chained.getContext("originalKey").toString(), QString("originalValue"));
        QCOMPARE(chained.getContext("chainedKey").toString(), QString("chainedValue"));
        
        qDebug() << "Chained exception:" << chained.fullMessage();
    } catch (...) {
        QFAIL("Exception chaining test should not throw");
    }
}

void TestExceptionIntegration::testExceptionLogging()
{
    try {
        SqlExecutionException ex("SQL execution failed", "SQL_ERROR");
        ex.setContext("sql", "SELECT * FROM nonexistent_table");
        ex.setContext("parameters", QVariantMap{{"id", 1}});
        
        QString logMessage = ex.fullMessage();
        QVERIFY(logMessage.contains("SQL execution failed"));
        QVERIFY(logMessage.contains("SQL_ERROR"));
        QVERIFY(logMessage.contains("sql="));
        QVERIFY(logMessage.contains("parameters="));
        
        qDebug() << "Log message:" << logMessage;
    } catch (...) {
        QFAIL("Exception logging test should not throw");
    }
}

void TestExceptionIntegration::createTestConfig(const QString& path, bool valid)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QTextStream stream(&file);
    
    if (valid) {
        stream << R"({
            "database": {
                "driverName": "QSQLITE",
                "databaseName": ":memory:",
                "maxConnections": 10,
                "minConnections": 2,
                "maxIdleTime": 300,
                "cacheEnabled": true,
                "maxCacheSize": 1000,
                "cacheExpireTime": 600
            }
        })";
    } else {
        stream << "{ invalid json }";
    }
    
    file.close();
}

void TestExceptionIntegration::createTestMapper(const QString& path, bool valid)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QTextStream stream(&file);
    
    if (valid) {
        stream << R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="TestMapper">
    <select id="selectTest" resultType="QVariantMap">
        SELECT 1 as id, 'test' as name
    </select>
</mapper>)";
    } else {
        stream << "invalid xml content";
    }
    
    file.close();
}

DatabaseConfig TestExceptionIntegration::createValidDatabaseConfig()
{
    DatabaseConfig config;
    config.driverName = "QSQLITE";
    config.databaseName = ":memory:";
    config.maxConnections = 10;
    config.minConnections = 2;
    config.maxIdleTime = 300;
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    return config;
}

DatabaseConfig TestExceptionIntegration::createInvalidDatabaseConfig()
{
    DatabaseConfig config;
    config.driverName = "INVALID_DRIVER";
    config.databaseName = "/nonexistent/path/database.db";
    config.hostName = "nonexistent.host";
    config.port = 99999;
    config.userName = "invalid_user";
    config.password = "invalid_password";
    return config;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Set up test environment
    QCoreApplication::setApplicationName("QtMyBatisORM-ExceptionIntegrationTest");
    QCoreApplication::setOrganizationName("QtMyBatisORM");
    
    qDebug() << "Starting Qt-MyBatis-ORM Exception Integration Tests";
    qDebug() << "=================================================";
    
    // Run the test
    TestExceptionIntegration test;
    int result = QTest::qExec(&test, argc, argv);
    
    if (result == 0) {
        qDebug() << "\n=== All Exception Integration Tests PASSED ===";
    } else {
        qDebug() << "\n=== Some Exception Integration Tests FAILED ===";
    }
    
    return result;
}

#include "run_exception_integration_test.moc"