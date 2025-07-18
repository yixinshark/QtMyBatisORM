#include <QtTest>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/DataModels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestJSONConfigParser : public QObject
{
public:
    void testParseValidConfiguration();
    void testParseMinimalConfiguration();
    void testParseInvalidJSON();
    void testParseInvalidDriver();
    void testParseEmptyDatabase();
    void testParseInvalidConnectionPool();

private:
    JSONConfigParser m_parser;
    QString createTempJsonFile(const QString& content);
};

void TestJSONConfigParser::testParseValidConfiguration()
{
    QString jsonContent = R"({
        "driver": "QSQLITE",
        "host": "localhost",
        "port": 3306,
        "database": "test_db",
        "username": "test_user",
        "password": "test_pass",
        "connectionPool": {
            "maxConnections": 15,
            "minConnections": 3,
            "maxIdleTime": 600
        },
        "cache": {
            "enabled": true,
            "maxSize": 2000,
            "expireTime": 900
        }
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    try {
        DatabaseConfig config = m_parser.parseConfiguration(tempFile);
        
        QCOMPARE(config.driverName, QString("QSQLITE"));
        QCOMPARE(config.hostName, QString("localhost"));
        QCOMPARE(config.port, 3306);
        QCOMPARE(config.databaseName, QString("test_db"));
        QCOMPARE(config.userName, QString("test_user"));
        QCOMPARE(config.password, QString("test_pass"));
        QCOMPARE(config.maxConnections, 15);
        QCOMPARE(config.minConnections, 3);
        QCOMPARE(config.maxIdleTime, 600);
        QVERIFY(config.cacheEnabled);
        QCOMPARE(config.maxCacheSize, 2000);
        QCOMPARE(config.cacheExpireTime, 900);
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestJSONConfigParser::testParseMinimalConfiguration()
{
    QString jsonContent = R"({
        "driver": "QMYSQL",
        "database": "minimal_db"
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    try {
        DatabaseConfig config = m_parser.parseConfiguration(tempFile);
        
        QCOMPARE(config.driverName, QString("QMYSQL"));
        QCOMPARE(config.hostName, QString("localhost")); // 默认值
        QCOMPARE(config.port, 3306); // 默认值
        QCOMPARE(config.databaseName, QString("minimal_db"));
        QCOMPARE(config.maxConnections, 10); // 默认值
        QCOMPARE(config.minConnections, 2); // 默认值
        QVERIFY(config.cacheEnabled); // 默认值
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestJSONConfigParser::testParseInvalidJSON()
{
    QString jsonContent = R"({
        "driver": "QSQLITE",
        "database": "test_db"
        // Missing comma - invalid JSON
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseInvalidDriver()
{
    QString jsonContent = R"({
        "driver": "INVALID_DRIVER",
        "database": "test_db"
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseEmptyDatabase()
{
    QString jsonContent = R"({
        "driver": "QSQLITE",
        "database": ""
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseInvalidConnectionPool()
{
    QString jsonContent = R"({
        "driver": "QSQLITE",
        "database": "test_db",
        "connectionPool": {
            "maxConnections": -1,
            "minConnections": 5
        }
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    if (tempFile.isEmpty()) {
        QFAIL("Failed to create temporary file");
        return;
    }
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

QString TestJSONConfigParser::createTempJsonFile(const QString& content)
{
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setAutoRemove(false);
    
    if (!tempFile->open()) {
        delete tempFile;
        return QString();
    }
    
    tempFile->write(content.toUtf8());
    tempFile->close();
    
    return tempFile->fileName();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestJSONConfigParser test;
    
    qDebug() << "Running JSON Config Parser Tests...";
    
    try {
        qDebug() << "Test 1: Valid Configuration";
        test.testParseValidConfiguration();
        qDebug() << "✓ PASSED";
        
        qDebug() << "Test 2: Minimal Configuration";
        test.testParseMinimalConfiguration();
        qDebug() << "✓ PASSED";
        
        qDebug() << "Test 3: Invalid JSON";
        test.testParseInvalidJSON();
        qDebug() << "✓ PASSED";
        
        qDebug() << "Test 4: Invalid Driver";
        test.testParseInvalidDriver();
        qDebug() << "✓ PASSED";
        
        qDebug() << "Test 5: Empty Database";
        test.testParseEmptyDatabase();
        qDebug() << "✓ PASSED";
        
        qDebug() << "Test 6: Invalid Connection Pool";
        test.testParseInvalidConnectionPool();
        qDebug() << "✓ PASSED";
        
        qDebug() << "\nAll tests passed successfully!";
        return 0;
    } catch (const std::exception& e) {
        qDebug() << "✗ Test failed with exception:" << e.what();
        return 1;
    } catch (...) {
        qDebug() << "✗ Test failed with unknown exception";
        return 1;
    }
}