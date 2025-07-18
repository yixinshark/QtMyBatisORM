#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/DataModels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestJSONConfigParser : public QObject
{
    Q_OBJECT

private slots:
    void testParseValidConfiguration();
    void testParseMinimalConfiguration();
    void testParseInvalidJSON();
    void testParseInvalidDriver();
    void testParseEmptyDatabase();
    void testParseInvalidConnectionPool();
    void testParseFromJsonObject();

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
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseInvalidDriver()
{
    QString jsonContent = R"({
        "driver": "INVALID_DRIVER",
        "database": "test_db"
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseEmptyDatabase()
{
    QString jsonContent = R"({
        "driver": "QSQLITE",
        "database": ""
    })";
    
    QString tempFile = createTempJsonFile(jsonContent);
    
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
    
    QVERIFY_EXCEPTION_THROWN(m_parser.parseConfiguration(tempFile), ConfigurationException);
}

void TestJSONConfigParser::testParseFromJsonObject()
{
    QJsonObject jsonObj;
    jsonObj["driver"] = "QMYSQL";
    jsonObj["database"] = "test_db";
    jsonObj["host"] = "testhost";
    jsonObj["port"] = 5432;
    
    QJsonObject poolConfig;
    poolConfig["maxConnections"] = 20;
    poolConfig["minConnections"] = 5;
    jsonObj["connectionPool"] = poolConfig;
    
    // 由于parseFromJsonObject是私有方法，我们通过公共接口测试
    QString jsonContent = QString::fromUtf8(QJsonDocument(jsonObj).toJson());
    QString tempFile = createTempJsonFile(jsonContent);
    
    try {
        DatabaseConfig config = m_parser.parseConfiguration(tempFile);
        
        QCOMPARE(config.driverName, QString("QMYSQL"));
        QCOMPARE(config.databaseName, QString("test_db"));
        QCOMPARE(config.hostName, QString("testhost"));
        QCOMPARE(config.port, 5432);
        QCOMPARE(config.maxConnections, 20);
        QCOMPARE(config.minConnections, 5);
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
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

#include "test_jsonconfigparser.moc"