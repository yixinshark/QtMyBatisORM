#include <QtTest/QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestQtMyBatisORM : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 基本功能测试
    void testConstructorAndDestructor();
    void testInitializeWithValidConfig();
    void testInitializeWithInvalidConfig();
    void testInitializeWithDatabaseConfig();
    void testGetSessionFactory();
    void testOpenAndCloseSession();
    void testIsInitialized();
    void testGetDatabaseConfig();
    void testShutdown();
    
    // 静态工厂方法测试
    void testStaticCreateMethod();
    void testStaticCreateWithConfigMethod();
    void testStaticCreateWithInvalidConfig();
    
    // 错误处理测试
    void testUninitializedAccess();
    void testDoubleInitialization();

private:
    QString createValidConfigFile();
    QString createInvalidConfigFile();
    DatabaseConfig createValidDatabaseConfig();
    
    QTemporaryFile* m_validConfigFile;
    QTemporaryFile* m_invalidConfigFile;
};

void TestQtMyBatisORM::initTestCase()
{
    // 创建测试用的配置文件
    m_validConfigFile = new QTemporaryFile(this);
    m_validConfigFile->setFileTemplate("valid_config_XXXXXX.json");
    m_validConfigFile->open();
    
    m_invalidConfigFile = new QTemporaryFile(this);
    m_invalidConfigFile->setFileTemplate("invalid_config_XXXXXX.json");
    m_invalidConfigFile->open();
}

void TestQtMyBatisORM::cleanupTestCase()
{
    delete m_validConfigFile;
    delete m_invalidConfigFile;
}

void TestQtMyBatisORM::init()
{
    // 每个测试前的初始化
}

void TestQtMyBatisORM::cleanup()
{
    // 每个测试后的清理
}

void TestQtMyBatisORM::testConstructorAndDestructor()
{
    QtMyBatisORM::QtMyBatisORM* orm = new QtMyBatisORM::QtMyBatisORM();
    QVERIFY(orm != nullptr);
    QVERIFY(!orm->isInitialized());
    delete orm;
}

void TestQtMyBatisORM::testInitializeWithValidConfig()
{
    QtMyBatisORM::QtMyBatisORM orm;
    QString configPath = createValidConfigFile();
    
    bool result = orm.initialize(configPath);
    QVERIFY(result);
    QVERIFY(orm.isInitialized());
    
    // 验证配置是否正确加载
    DatabaseConfig config = orm.getDatabaseConfig();
    QCOMPARE(config.driverName, QString("QSQLITE"));
    QCOMPARE(config.databaseName, QString(":memory:"));
}

void TestQtMyBatisORM::testInitializeWithInvalidConfig()
{
    QtMyBatisORM::QtMyBatisORM orm;
    QString configPath = createInvalidConfigFile();
    
    bool result = orm.initialize(configPath);
    QVERIFY(!result);
    QVERIFY(!orm.isInitialized());
}

void TestQtMyBatisORM::testInitializeWithDatabaseConfig()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    
    bool result = orm.initializeWithConfig(config);
    QVERIFY(result);
    QVERIFY(orm.isInitialized());
    
    DatabaseConfig retrievedConfig = orm.getDatabaseConfig();
    QCOMPARE(retrievedConfig.driverName, config.driverName);
    QCOMPARE(retrievedConfig.databaseName, config.databaseName);
}

void TestQtMyBatisORM::testGetSessionFactory()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    
    QVERIFY(orm.initializeWithConfig(config));
    
    QSharedPointer<SessionFactory> factory = orm.getSessionFactory();
    QVERIFY(!factory.isNull());
    QVERIFY(!factory->isClosed());
}

void TestQtMyBatisORM::testOpenAndCloseSession()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    
    QVERIFY(orm.initializeWithConfig(config));
    
    QSharedPointer<Session> session = orm.openSession();
    QVERIFY(!session.isNull());
    
    orm.closeSession(session);
    // Session应该仍然有效，但连接已归还到池中
    QVERIFY(!session.isNull());
}

void TestQtMyBatisORM::testIsInitialized()
{
    QtMyBatisORM::QtMyBatisORM orm;
    QVERIFY(!orm.isInitialized());
    
    DatabaseConfig config = createValidDatabaseConfig();
    orm.initializeWithConfig(config);
    QVERIFY(orm.isInitialized());
    
    orm.shutdown();
    QVERIFY(!orm.isInitialized());
}

void TestQtMyBatisORM::testGetDatabaseConfig()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    config.maxConnections = 15;
    config.minConnections = 3;
    
    orm.initializeWithConfig(config);
    
    DatabaseConfig retrievedConfig = orm.getDatabaseConfig();
    QCOMPARE(retrievedConfig.driverName, config.driverName);
    QCOMPARE(retrievedConfig.databaseName, config.databaseName);
    QCOMPARE(retrievedConfig.maxConnections, config.maxConnections);
    QCOMPARE(retrievedConfig.minConnections, config.minConnections);
}

void TestQtMyBatisORM::testShutdown()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    
    orm.initializeWithConfig(config);
    QVERIFY(orm.isInitialized());
    
    QSharedPointer<SessionFactory> factory = orm.getSessionFactory();
    QVERIFY(!factory.isNull());
    
    orm.shutdown();
    QVERIFY(!orm.isInitialized());
    QVERIFY(factory->isClosed());
}

void TestQtMyBatisORM::testStaticCreateMethod()
{
    QString configPath = createValidConfigFile();
    
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::create(configPath);
    QVERIFY(!orm.isNull());
    QVERIFY(orm->isInitialized());
    
    DatabaseConfig config = orm->getDatabaseConfig();
    QCOMPARE(config.driverName, QString("QSQLITE"));
}

void TestQtMyBatisORM::testStaticCreateWithConfigMethod()
{
    DatabaseConfig config = createValidDatabaseConfig();
    
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createWithConfig(config);
    QVERIFY(!orm.isNull());
    QVERIFY(orm->isInitialized());
    
    DatabaseConfig retrievedConfig = orm->getDatabaseConfig();
    QCOMPARE(retrievedConfig.driverName, config.driverName);
}

void TestQtMyBatisORM::testStaticCreateWithInvalidConfig()
{
    QString configPath = createInvalidConfigFile();
    
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::create(configPath);
    QVERIFY(orm.isNull());
}

void TestQtMyBatisORM::testUninitializedAccess()
{
    QtMyBatisORM::QtMyBatisORM orm;
    
    // 未初始化时访问SessionFactory应该抛出异常
    QVERIFY_EXCEPTION_THROWN(orm.getSessionFactory(), ConfigurationException);
    
    // 未初始化时打开Session应该抛出异常
    QVERIFY_EXCEPTION_THROWN(orm.openSession(), ConfigurationException);
}

void TestQtMyBatisORM::testDoubleInitialization()
{
    QtMyBatisORM::QtMyBatisORM orm;
    DatabaseConfig config = createValidDatabaseConfig();
    
    // 第一次初始化
    QVERIFY(orm.initializeWithConfig(config));
    QVERIFY(orm.isInitialized());
    
    // 第二次初始化应该成功（重新初始化）
    config.maxConnections = 20;
    QVERIFY(orm.initializeWithConfig(config));
    QVERIFY(orm.isInitialized());
    
    DatabaseConfig retrievedConfig = orm.getDatabaseConfig();
    QCOMPARE(retrievedConfig.maxConnections, 20);
}

QString TestQtMyBatisORM::createValidConfigFile()
{
    QJsonObject config;
    config["driverName"] = "QSQLITE";
    config["databaseName"] = ":memory:";
    config["maxConnections"] = 10;
    config["minConnections"] = 2;
    config["maxIdleTime"] = 300;
    config["cacheEnabled"] = true;
    config["maxCacheSize"] = 1000;
    config["cacheExpireTime"] = 600;
    
    QJsonDocument doc(config);
    
    m_validConfigFile->resize(0);
    m_validConfigFile->write(doc.toJson());
    m_validConfigFile->flush();
    
    return m_validConfigFile->fileName();
}

QString TestQtMyBatisORM::createInvalidConfigFile()
{
    // 创建格式错误的JSON文件
    QString invalidJson = "{ invalid json content }";
    
    m_invalidConfigFile->resize(0);
    m_invalidConfigFile->write(invalidJson.toUtf8());
    m_invalidConfigFile->flush();
    
    return m_invalidConfigFile->fileName();
}

DatabaseConfig TestQtMyBatisORM::createValidDatabaseConfig()
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

#include "test_qtmybatisorm.moc"

