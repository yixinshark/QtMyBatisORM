#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/datamodels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestNewConfigFormat : public QObject
{
    Q_OBJECT

private slots:
    void testNewConfigFormat();
    void testNewConfigFormatValidation();
    void testNewConfigFormatDefaults();
    void testNewConfigFormatSqlFiles();
    void testInvalidNewConfigFormat();

private:
    QString createTempConfigFile(const QString& content);
    QStringList m_tempFiles;
};

void TestNewConfigFormat::testNewConfigFormat()
{
    QString jsonContent = R"({
        "database": {
            "debug": true,
            "type": "QMYSQL",
            "host": "192.168.1.100",
            "port": 3306,
            "database_name": "test_db",
            "username": "test_user",
            "password": "test_pass",
            "max_wait_time": 8000,
            "max_connection_count": 15,
            "sql_files": [
                ":/sql/user.sql",
                ":/sql/product.sql",
                ":/sql/order.sql"
            ]
        }
    })";
    
    QString tempFile = createTempConfigFile(jsonContent);
    
    try {
        JSONConfigParser parser;
        DatabaseConfig config = parser.parseConfiguration(tempFile);
        
        // 验证基本配置
        QCOMPARE(config.driverName, QString("QMYSQL"));
        QCOMPARE(config.hostName, QString("192.168.1.100"));
        QCOMPARE(config.port, 3306);
        QCOMPARE(config.databaseName, QString("test_db"));
        QCOMPARE(config.userName, QString("test_user"));
        QCOMPARE(config.password, QString("test_pass"));
        
        // 验证调试配置
        QVERIFY(config.debug);
        
        // 验证连接池配置
        QCOMPARE(config.maxConnections, 15);
        QCOMPARE(config.maxWaitTime, 8000);
        
        // 验证SQL文件列表
        QCOMPARE(config.sqlFiles.size(), 3);
        QCOMPARE(config.sqlFiles[0], QString(":/sql/user.sql"));
        QCOMPARE(config.sqlFiles[1], QString(":/sql/product.sql"));
        QCOMPARE(config.sqlFiles[2], QString(":/sql/order.sql"));
        
        // 验证固定的默认值
        QCOMPARE(config.minConnections, 2);
        QCOMPARE(config.maxIdleTime, 300);
        QVERIFY(config.cacheEnabled);
        QCOMPARE(config.maxCacheSize, 1000);
        QCOMPARE(config.cacheExpireTime, 600);
        
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestNewConfigFormat::testNewConfigFormatValidation()
{
    // 测试QSQLITE配置
    QString sqliteConfig = R"({
        "database": {
            "debug": false,
            "type": "QSQLITE",
            "database_name": "/tmp/test.db",
            "max_connection_count": 5,
            "sql_files": [":/sql/test.sql"]
        }
    })";
    
    QString tempFile = createTempConfigFile(sqliteConfig);
    
    try {
        JSONConfigParser parser;
        DatabaseConfig config = parser.parseConfiguration(tempFile);
        
        QCOMPARE(config.driverName, QString("QSQLITE"));
        QCOMPARE(config.databaseName, QString("/tmp/test.db"));
        QVERIFY(!config.debug);
        QCOMPARE(config.maxConnections, 5);
        QCOMPARE(config.sqlFiles.size(), 1);
        
        // 验证默认值
        QCOMPARE(config.hostName, QString("localhost"));
        QCOMPARE(config.port, 3306);
        QCOMPARE(config.maxWaitTime, 5000);
        
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestNewConfigFormat::testNewConfigFormatDefaults()
{
    // 测试最小配置（使用所有默认值）
    QString minimalConfig = R"({
        "database": {
            "type": "QSQLITE",
            "database_name": ":memory:",
            "sql_files": []
        }
    })";
    
    QString tempFile = createTempConfigFile(minimalConfig);
    
    try {
        JSONConfigParser parser;
        DatabaseConfig config = parser.parseConfiguration(tempFile);
        
        // 验证必填字段
        QCOMPARE(config.driverName, QString("QSQLITE"));
        QCOMPARE(config.databaseName, QString(":memory:"));
        
        // 验证默认值
        QVERIFY(!config.debug); // 默认false
        QCOMPARE(config.hostName, QString("localhost"));
        QCOMPARE(config.port, 3306);
        QVERIFY(config.userName.isEmpty());
        QVERIFY(config.password.isEmpty());
        QCOMPARE(config.maxConnections, 10);
        QCOMPARE(config.maxWaitTime, 5000);
        QVERIFY(config.sqlFiles.isEmpty());
        
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestNewConfigFormat::testNewConfigFormatSqlFiles()
{
    // 测试包含多个SQL文件的配置
    QString configWithSqlFiles = R"({
        "database": {
            "type": "QMYSQL",
            "host": "localhost",
            "database_name": "test_db",
            "username": "user",
            "password": "pass",
            "sql_files": [
                ":/sql/users.sql",
                ":/sql/products.sql",
                ":/sql/orders.sql",
                ":/sql/categories.sql",
                ":/sql/reviews.sql"
            ]
        }
    })";
    
    QString tempFile = createTempConfigFile(configWithSqlFiles);
    
    try {
        JSONConfigParser parser;
        DatabaseConfig config = parser.parseConfiguration(tempFile);
        
        QCOMPARE(config.sqlFiles.size(), 5);
        QCOMPARE(config.sqlFiles[0], QString(":/sql/users.sql"));
        QCOMPARE(config.sqlFiles[1], QString(":/sql/products.sql"));
        QCOMPARE(config.sqlFiles[2], QString(":/sql/orders.sql"));
        QCOMPARE(config.sqlFiles[3], QString(":/sql/categories.sql"));
        QCOMPARE(config.sqlFiles[4], QString(":/sql/reviews.sql"));
        
    } catch (const ConfigurationException& e) {
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toLocal8Bit());
    }
}

void TestNewConfigFormat::testInvalidNewConfigFormat()
{
    // 测试缺少database对象
    QString invalidConfig1 = R"({
        "type": "QMYSQL",
        "host": "localhost"
    })";
    
    QString tempFile1 = createTempConfigFile(invalidConfig1);
    
    try {
        JSONConfigParser parser;
        parser.parseConfiguration(tempFile1);
        QFAIL("Should throw exception for missing database object");
    } catch (const ConfigurationException& e) {
        QVERIFY(QString(e.what()).contains("database"));
    }
    
    // 测试空的database对象
    QString invalidConfig2 = R"({
        "database": {}
    })";
    
    QString tempFile2 = createTempConfigFile(invalidConfig2);
    
    try {
        JSONConfigParser parser;
        parser.parseConfiguration(tempFile2);
        QFAIL("Should throw exception for empty database object");
    } catch (const ConfigurationException& e) {
        QVERIFY(QString(e.what()).contains("empty"));
    }
    
    // 测试无效的数据库类型
    QString invalidConfig3 = R"({
        "database": {
            "type": "INVALID_DB_TYPE",
            "database_name": "test"
        }
    })";
    
    QString tempFile3 = createTempConfigFile(invalidConfig3);
    
    try {
        JSONConfigParser parser;
        parser.parseConfiguration(tempFile3);
        QFAIL("Should throw exception for invalid database type");
    } catch (const ConfigurationException& e) {
        QVERIFY(QString(e.what()).contains("driver"));
    }
    
    // 测试无效的连接数配置
    QString invalidConfig4 = R"({
        "database": {
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 0
        }
    })";
    
    QString tempFile4 = createTempConfigFile(invalidConfig4);
    
    try {
        JSONConfigParser parser;
        parser.parseConfiguration(tempFile4);
        QFAIL("Should throw exception for invalid connection count");
    } catch (const ConfigurationException& e) {
        QVERIFY(QString(e.what()).contains("Max connections"));
    }
}

QString TestNewConfigFormat::createTempConfigFile(const QString& content)
{
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate("test_config_XXXXXX.json");
    tempFile->setAutoRemove(false);
    
    if (!tempFile->open()) {
        return QString();
    }
    
    tempFile->write(content.toUtf8());
    tempFile->close();
    
    QString fileName = tempFile->fileName();
    m_tempFiles.append(fileName);
    
    return fileName;
}

QTEST_GUILESS_MAIN(TestNewConfigFormat)
#include "test_new_config_format.moc" 