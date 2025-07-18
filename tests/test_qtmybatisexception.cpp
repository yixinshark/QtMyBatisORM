#include <QtTest/QtTest>
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestQtMyBatisException : public QObject
{
    Q_OBJECT

private slots:
    void testBasicException();
    void testExceptionWithDetail();
    void testExceptionContext();
    void testFullMessage();
    void testSpecificExceptions();
    void testExceptionCloning();
    void testExceptionRaising();
};

void TestQtMyBatisException::testBasicException()
{
    QtMyBatisException ex("Test message", "TEST_CODE");
    
    QCOMPARE(ex.message(), QString("Test message"));
    QCOMPARE(ex.code(), QString("TEST_CODE"));
    QVERIFY(ex.detail().isEmpty());
    QVERIFY(!ex.timestamp().isNull());
    QVERIFY(ex.getAllContext().isEmpty());
}

void TestQtMyBatisException::testExceptionWithDetail()
{
    QtMyBatisException ex("Test message", "TEST_CODE", "Detailed information");
    
    QCOMPARE(ex.message(), QString("Test message"));
    QCOMPARE(ex.code(), QString("TEST_CODE"));
    QCOMPARE(ex.detail(), QString("Detailed information"));
}

void TestQtMyBatisException::testExceptionContext()
{
    QtMyBatisException ex("Test message", "TEST_CODE");
    
    ex.setContext("key1", "value1");
    ex.setContext("key2", 42);
    ex.setContext("key3", true);
    
    QCOMPARE(ex.getContext("key1").toString(), QString("value1"));
    QCOMPARE(ex.getContext("key2").toInt(), 42);
    QCOMPARE(ex.getContext("key3").toBool(), true);
    QVERIFY(ex.getContext("nonexistent").isNull());
    
    QVariantMap context = ex.getAllContext();
    QCOMPARE(context.size(), 3);
    QVERIFY(context.contains("key1"));
    QVERIFY(context.contains("key2"));
    QVERIFY(context.contains("key3"));
}

void TestQtMyBatisException::testFullMessage()
{
    QtMyBatisException ex("Test message", "TEST_CODE", "Detail info");
    ex.setContext("operation", "select");
    ex.setContext("table", "users");
    
    QString fullMsg = ex.fullMessage();
    QVERIFY(fullMsg.contains("[TEST_CODE]"));
    QVERIFY(fullMsg.contains("Test message"));
    QVERIFY(fullMsg.contains("Detail info"));
    QVERIFY(fullMsg.contains("operation=select"));
    QVERIFY(fullMsg.contains("table=users"));
}

void TestQtMyBatisException::testSpecificExceptions()
{
    // Test ConfigurationException
    ConfigurationException configEx("Config error");
    QCOMPARE(configEx.code(), QString("CONFIG_ERROR"));
    QCOMPARE(configEx.message(), QString("Config error"));
    
    // Test SqlExecutionException
    SqlExecutionException sqlEx("SQL error");
    QCOMPARE(sqlEx.code(), QString("SQL_ERROR"));
    
    // Test ConnectionException
    ConnectionException connEx("Connection error");
    QCOMPARE(connEx.code(), QString("CONNECTION_ERROR"));
    
    // Test MappingException
    MappingException mapEx("Mapping error");
    QCOMPARE(mapEx.code(), QString("MAPPING_ERROR"));
    
    // Test CacheException
    CacheException cacheEx("Cache error");
    QCOMPARE(cacheEx.code(), QString("CACHE_ERROR"));
    
    // Test TransactionException
    TransactionException txEx("Transaction error");
    QCOMPARE(txEx.code(), QString("TRANSACTION_ERROR"));
    
    // Test ParameterException
    ParameterException paramEx("Parameter error");
    QCOMPARE(paramEx.code(), QString("PARAMETER_ERROR"));
    
    // Test ResultException
    ResultException resultEx("Result error");
    QCOMPARE(resultEx.code(), QString("RESULT_ERROR"));
    
    // Test SessionException
    SessionException sessionEx("Session error");
    QCOMPARE(sessionEx.code(), QString("SESSION_ERROR"));
}

void TestQtMyBatisException::testExceptionCloning()
{
    QtMyBatisException original("Original message", "ORIG_CODE", "Original detail");
    original.setContext("test", "value");
    
    std::unique_ptr<QtMyBatisException> cloned(original.clone());
    
    QCOMPARE(cloned->message(), original.message());
    QCOMPARE(cloned->code(), original.code());
    QCOMPARE(cloned->detail(), original.detail());
    QCOMPARE(cloned->getContext("test").toString(), QString("value"));
    
    // Test specific exception cloning
    ConfigurationException configOriginal("Config message");
    std::unique_ptr<ConfigurationException> configCloned(configOriginal.clone());
    QCOMPARE(configCloned->message(), configOriginal.message());
    QCOMPARE(configCloned->code(), configOriginal.code());
}

void TestQtMyBatisException::testExceptionRaising()
{
    QtMyBatisException ex("Test exception", "TEST_CODE");
    
    bool caught = false;
    try {
        ex.raise();
    } catch (const QtMyBatisException& e) {
        caught = true;
        QCOMPARE(e.message(), QString("Test exception"));
        QCOMPARE(e.code(), QString("TEST_CODE"));
    }
    
    QVERIFY(caught);
}

#include "test_qtmybatisexception.moc"

// Test implementation complete

