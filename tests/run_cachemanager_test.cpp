#include <QCoreApplication>
#include <QtTest>
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/datamodels.h"

using namespace QtMyBatisORM;

class TestCacheManager : public QObject
{
    Q_OBJECT

private slots:
    void testPutAndGet();
    void testRemove();
    void testClear();
    void testExpiration();
    void testDisabledCache();
    void testMaxSize();
    void testInvalidateByPattern();

private:
};

void TestCacheManager::testPutAndGet()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 60;
    
    CacheManager cache(config);
    
    cache.put("test_key", QVariant("test_value"));
    QVariant result = cache.get("test_key");
    
    QCOMPARE(result.toString(), QString("test_value"));
}

void TestCacheManager::testRemove()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    
    CacheManager cache(config);
    
    cache.put("test_key", QVariant("test_value"));
    QVERIFY(cache.contains("test_key"));
    
    cache.remove("test_key");
    QVERIFY(!cache.contains("test_key"));
}

void TestCacheManager::testClear()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    
    CacheManager cache(config);
    
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    
    QCOMPARE(cache.size(), 2);
    
    cache.clear();
    QCOMPARE(cache.size(), 0);
}

void TestCacheManager::testExpiration()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 1; // 1秒过期
    
    CacheManager cache(config);
    
    cache.put("test_key", QVariant("test_value"));
    QVERIFY(cache.contains("test_key"));
    
    // 立即检查，应该还没过期
    QVariant result1 = cache.get("test_key");
    QVERIFY(!result1.isNull());
    QCOMPARE(result1.toString(), QString("test_value"));
    
    // 等待过期
    QTest::qWait(1100); // 等待1.1秒
    
    // 尝试获取，应该返回空值（因为已过期）
    QVariant result2 = cache.get("test_key");
    QVERIFY(result2.isNull());
    
    // 检查缓存中是否还包含这个key（应该被自动清理）
    QVERIFY(!cache.contains("test_key"));
}

void TestCacheManager::testDisabledCache()
{
    DatabaseConfig config;
    config.cacheEnabled = false;
    
    CacheManager cache(config);
    
    QVERIFY(!cache.isEnabled());
    
    cache.put("test_key", QVariant("test_value"));
    QVariant result = cache.get("test_key");
    
    QVERIFY(result.isNull());
    QVERIFY(!cache.contains("test_key"));
    QCOMPARE(cache.size(), 0);
}

void TestCacheManager::testMaxSize()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 2; // 最大2个条目
    config.cacheExpireTime = 600;
    
    CacheManager cache(config);
    
    cache.put("key1", QVariant("value1"));
    QCOMPARE(cache.size(), 1);
    
    cache.put("key2", QVariant("value2"));
    QCOMPARE(cache.size(), 2);
    
    // 添加第三个条目，应该触发LRU清理
    cache.put("key3", QVariant("value3"));
    QCOMPARE(cache.size(), 2);
    
    // key1应该被清理掉（最久未访问）
    QVERIFY(!cache.contains("key1"));
    QVERIFY(cache.contains("key2"));
    QVERIFY(cache.contains("key3"));
}

void TestCacheManager::testInvalidateByPattern()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    
    CacheManager cache(config);
    
    cache.put("user:1", QVariant("user1"));
    cache.put("user:2", QVariant("user2"));
    cache.put("order:1", QVariant("order1"));
    cache.put("order:2", QVariant("order2"));
    
    QCOMPARE(cache.size(), 4);
    
    // 清理所有user相关的缓存
    cache.invalidateByPattern("user:.*");
    
    QCOMPARE(cache.size(), 2);
    QVERIFY(!cache.contains("user:1"));
    QVERIFY(!cache.contains("user:2"));
    QVERIFY(cache.contains("order:1"));
    QVERIFY(cache.contains("order:2"));
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestCacheManager test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_cachemanager_test.moc"