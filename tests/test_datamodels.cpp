#include <QtTest>
#include "QtMyBatisORM/DataModels.h"

using namespace QtMyBatisORM;

class TestDataModels : public QObject
{
    Q_OBJECT

private slots:
    void testDatabaseConfig();
    void testStatementConfig();
    void testMapperConfig();
    void testCacheEntry();

private:
};

void TestDataModels::testDatabaseConfig()
{
    DatabaseConfig config;
    config.driverName = "QMYSQL";
    config.hostName = "localhost";
    config.port = 3306;
    config.databaseName = "test";
    config.userName = "user";
    config.password = "pass";
    
    QCOMPARE(config.driverName, QString("QMYSQL"));
    QCOMPARE(config.hostName, QString("localhost"));
    QCOMPARE(config.port, 3306);
    QCOMPARE(config.maxConnections, 10); // 默认值
    QCOMPARE(config.minConnections, 2);  // 默认值
    QVERIFY(config.cacheEnabled);        // 默认值
}

void TestDataModels::testStatementConfig()
{
    StatementConfig config;
    config.id = "selectUser";
    config.sql = "SELECT * FROM users WHERE id = :id";
    config.type = StatementType::SELECT;
    config.parameterType = "int";
    config.resultType = "User";
    
    QCOMPARE(config.id, QString("selectUser"));
    QCOMPARE(config.type, StatementType::SELECT);
    QVERIFY(!config.useCache); // 默认值
}

void TestDataModels::testMapperConfig()
{
    MapperConfig config;
    config.namespace_ = "UserMapper";
    config.xmlPath = ":/mappers/UserMapper.xml";
    
    StatementConfig stmt;
    stmt.id = "selectById";
    stmt.type = StatementType::SELECT;
    config.statements["selectById"] = stmt;
    
    QCOMPARE(config.namespace_, QString("UserMapper"));
    QVERIFY(config.statements.contains("selectById"));
}

void TestDataModels::testCacheEntry()
{
    CacheEntry entry;
    entry.value = QVariant("test data");
    entry.timestamp = QDateTime::currentDateTime();
    entry.accessCount = 1;
    
    QCOMPARE(entry.value.toString(), QString("test data"));
    QCOMPARE(entry.accessCount, 1);
    QVERIFY(entry.timestamp.isValid());
}

#include "test_datamodels.moc"