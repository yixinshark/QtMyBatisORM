#include <QtTest>
#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/datamodels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestConfigurationManagerStandalone : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingleton();
    void testInitialState();
    void testReset();
    void testConvenienceMethods();

private:
    ConfigurationManager* m_configManager;
};

void TestConfigurationManagerStandalone::initTestCase()
{
    m_configManager = ConfigurationManager::instance();
    QVERIFY(m_configManager != nullptr);
}

void TestConfigurationManagerStandalone::cleanupTestCase()
{
    if (m_configManager) {
        m_configManager->reset();
    }
}

void TestConfigurationManagerStandalone::testSingleton()
{
    ConfigurationManager* instance1 = ConfigurationManager::instance();
    ConfigurationManager* instance2 = ConfigurationManager::instance();
    
    QCOMPARE(instance1, instance2);
    QVERIFY(instance1 != nullptr);
    QCOMPARE(instance1, m_configManager);
}

void TestConfigurationManagerStandalone::testInitialState()
{
    // 重置状态
    m_configManager->reset();
    
    // 测试初始状态
    QVERIFY(!m_configManager->isConfigured());
    QCOMPARE(m_configManager->getMapperCount(), 0);
    QVERIFY(m_configManager->getMapperNamespaces().isEmpty());
    
    // 测试获取不存在的Mapper配置
    QVERIFY(!m_configManager->hasMapper("NonExistentMapper"));
    
    MapperConfig config = m_configManager->getMapperConfig("NonExistentMapper");
    QVERIFY(config.namespace_.isEmpty());
    
    // 测试获取数据库配置（应该是默认值）
    DatabaseConfig dbConfig = m_configManager->getDatabaseConfig();
    QVERIFY(dbConfig.driverName.isEmpty());
    QVERIFY(dbConfig.databaseName.isEmpty());
}

void TestConfigurationManagerStandalone::testReset()
{
    // 重置应该清空所有配置
    m_configManager->reset();
    
    QVERIFY(!m_configManager->isConfigured());
    QCOMPARE(m_configManager->getMapperCount(), 0);
    QVERIFY(m_configManager->getMapperNamespaces().isEmpty());
    QVERIFY(m_configManager->getMapperConfigs().isEmpty());
}

void TestConfigurationManagerStandalone::testConvenienceMethods()
{
    // 重置状态
    m_configManager->reset();
    
    // 测试便利方法
    QVERIFY(!m_configManager->isConfigured());
    QCOMPARE(m_configManager->getMapperCount(), 0);
    QVERIFY(m_configManager->getMapperNamespaces().isEmpty());
    
    // 测试空路径处理 - 应该抛出异常
    bool caughtConfigException = false;
    try {
        m_configManager->loadConfiguration("");
    } catch (const ConfigurationException& e) {
        caughtConfigException = true;
        QCOMPARE(e.code(), QLatin1String("CONFIG_EMPTY_PATH"));
    }
    QVERIFY(caughtConfigException);
    
    caughtConfigException = false;
    try {
        m_configManager->loadMappers(QStringList());
    } catch (const ConfigurationException& e) {
        caughtConfigException = true;
        QCOMPARE(e.code(), QLatin1String("CONFIG_EMPTY_MAPPER_PATHS"));
    }
    QVERIFY(caughtConfigException);
}

QTEST_MAIN(TestConfigurationManagerStandalone)
#include "run_configurationmanager_test.moc"