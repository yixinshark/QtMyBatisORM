#include <QtTest>
#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/DataModels.h"

using namespace QtMyBatisORM;

class TestConfigurationManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingleton();
    void testLoadConfiguration();
    void testLoadMappers();
    void testGetMapperConfig();

private:
    ConfigurationManager* m_configManager;
};

void TestConfigurationManager::initTestCase()
{
    m_configManager = ConfigurationManager::instance();
}

void TestConfigurationManager::cleanupTestCase()
{
    if (m_configManager) {
        m_configManager->reset();
    }
}

void TestConfigurationManager::testSingleton()
{
    ConfigurationManager* instance1 = ConfigurationManager::instance();
    ConfigurationManager* instance2 = ConfigurationManager::instance();
    
    QCOMPARE(instance1, instance2);
    QVERIFY(instance1 != nullptr);
}

void TestConfigurationManager::testLoadConfiguration()
{
    // 这个测试需要实际的配置文件
    // 暂时跳过，因为我们还没有创建测试资源文件
    QSKIP("Skipping configuration loading test - no test resources available");
}

void TestConfigurationManager::testLoadMappers()
{
    // 这个测试需要实际的Mapper文件
    // 暂时跳过，因为我们还没有创建测试资源文件
    QSKIP("Skipping mapper loading test - no test resources available");
}

void TestConfigurationManager::testGetMapperConfig()
{
    // 测试获取不存在的Mapper配置
    QVERIFY(!m_configManager->hasMapper("NonExistentMapper"));
    
    MapperConfig config = m_configManager->getMapperConfig("NonExistentMapper");
    QVERIFY(config.namespace_.isEmpty());
}

#include "test_configurationmanager.moc"