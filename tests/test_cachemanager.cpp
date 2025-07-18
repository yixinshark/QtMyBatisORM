#include <QtTest>
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/DataModels.h"

using namespace QtMyBatisORM;

class TestCacheManager : public QObject
{
    Q_OBJECT

private slots:
    void testPutAndGet();
    void testRemove();
    void testClear();
    void testExpiration();
    
    // LRU策略测试
    void testLRUEviction();
    void testLRUAccessOrder();
    void testLRUCapacityControl();
    void testLRUWithMixedOperations();
    
    // 统计功能测试
    void testCacheStatistics();
    void testHitRateCalculation();
    void testEvictionStatistics();
    void testStatisticsReset();

private:
    DatabaseConfig createTestConfig(int maxSize = 100, int expireTime = 60);
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
    QSKIP("Skipping expiration test - requires time-based testing");
}

DatabaseConfig TestCacheManager::createTestConfig(int maxSize, int expireTime)
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = maxSize;
    config.cacheExpireTime = expireTime;
    return config;
}

void TestCacheManager::testLRUEviction()
{
    // 测试LRU驱逐策略
    DatabaseConfig config = createTestConfig(3); // 最大容量为3
    CacheManager cache(config);
    
    // 添加3个条目，填满缓存
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    cache.put("key3", QVariant("value3"));
    
    QCOMPARE(cache.size(), 3);
    
    // 添加第4个条目，应该驱逐最久未使用的条目(key1)
    cache.put("key4", QVariant("value4"));
    
    QCOMPARE(cache.size(), 3);
    QVERIFY(!cache.contains("key1")); // key1应该被驱逐
    QVERIFY(cache.contains("key2"));
    QVERIFY(cache.contains("key3"));
    QVERIFY(cache.contains("key4"));
}

void TestCacheManager::testLRUAccessOrder()
{
    // 测试访问顺序对LRU的影响
    DatabaseConfig config = createTestConfig(3);
    CacheManager cache(config);
    
    // 添加3个条目
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    cache.put("key3", QVariant("value3"));
    
    // 访问key1，使其成为最近使用的
    cache.get("key1");
    
    // 添加新条目，应该驱逐key2（最久未访问的）
    cache.put("key4", QVariant("value4"));
    
    QVERIFY(cache.contains("key1")); // key1被访问过，不应该被驱逐
    QVERIFY(!cache.contains("key2")); // key2应该被驱逐
    QVERIFY(cache.contains("key3"));
    QVERIFY(cache.contains("key4"));
}

void TestCacheManager::testLRUCapacityControl()
{
    // 测试容量控制
    DatabaseConfig config = createTestConfig(5);
    CacheManager cache(config);
    
    // 添加5个条目
    for (int i = 1; i <= 5; ++i) {
        cache.put(QString("key%1").arg(i), QVariant(QString("value%1").arg(i)));
    }
    
    QCOMPARE(cache.size(), 5);
    
    // 添加更多条目，验证容量控制
    for (int i = 6; i <= 10; ++i) {
        cache.put(QString("key%1").arg(i), QVariant(QString("value%1").arg(i)));
        QVERIFY(cache.size() <= 5); // 不应该超过最大容量
    }
    
    // 验证最新的条目存在
    QVERIFY(cache.contains("key10"));
    QVERIFY(cache.contains("key9"));
    QVERIFY(cache.contains("key8"));
    QVERIFY(cache.contains("key7"));
    QVERIFY(cache.contains("key6"));
    
    // 验证最旧的条目被驱逐
    QVERIFY(!cache.contains("key1"));
    QVERIFY(!cache.contains("key2"));
    QVERIFY(!cache.contains("key3"));
    QVERIFY(!cache.contains("key4"));
    QVERIFY(!cache.contains("key5"));
}

void TestCacheManager::testLRUWithMixedOperations()
{
    // 测试混合操作下的LRU行为
    DatabaseConfig config = createTestConfig(4);
    CacheManager cache(config);
    
    // 添加4个条目
    cache.put("A", QVariant("valueA"));
    cache.put("B", QVariant("valueB"));
    cache.put("C", QVariant("valueC"));
    cache.put("D", QVariant("valueD"));
    
    // 访问A和C，改变访问顺序
    cache.get("A");
    cache.get("C");
    
    // 添加新条目E，应该驱逐B（最久未访问）
    cache.put("E", QVariant("valueE"));
    
    QVERIFY(cache.contains("A"));
    QVERIFY(!cache.contains("B")); // B应该被驱逐
    QVERIFY(cache.contains("C"));
    QVERIFY(cache.contains("D"));
    QVERIFY(cache.contains("E"));
    
    // 再次访问D
    cache.get("D");
    
    // 添加新条目F，现在应该驱逐E（相对最久未访问）
    cache.put("F", QVariant("valueF"));
    
    QVERIFY(cache.contains("A"));
    QVERIFY(cache.contains("C"));
    QVERIFY(cache.contains("D"));
    QVERIFY(!cache.contains("E")); // E应该被驱逐
    QVERIFY(cache.contains("F"));
}

void TestCacheManager::testCacheStatistics()
{
    // 测试缓存统计功能
    DatabaseConfig config = createTestConfig(10);
    CacheManager cache(config);
    
    // 初始统计应该为空
    CacheStats stats = cache.getStats();
    QCOMPARE(stats.totalRequests, 0);
    QCOMPARE(stats.hitCount, 0);
    QCOMPARE(stats.missCount, 0);
    QCOMPARE(stats.currentSize, 0);
    QCOMPARE(stats.maxSize, 10);
    
    // 添加一些数据
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    
    stats = cache.getStats();
    QCOMPARE(stats.currentSize, 2);
    
    // 测试命中和未命中
    cache.get("key1"); // 命中
    cache.get("key3"); // 未命中
    
    stats = cache.getStats();
    QCOMPARE(stats.totalRequests, 2);
    QCOMPARE(stats.hitCount, 1);
    QCOMPARE(stats.missCount, 1);
}

void TestCacheManager::testHitRateCalculation()
{
    // 测试命中率计算
    DatabaseConfig config = createTestConfig(10);
    CacheManager cache(config);
    
    // 添加数据
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    cache.put("key3", QVariant("value3"));
    
    // 执行一些查询操作
    cache.get("key1"); // 命中
    cache.get("key2"); // 命中
    cache.get("key4"); // 未命中
    cache.get("key1"); // 命中
    cache.get("key5"); // 未命中
    
    CacheStats stats = cache.getStats();
    QCOMPARE(stats.totalRequests, 5);
    QCOMPARE(stats.hitCount, 3);
    QCOMPARE(stats.missCount, 2);
    
    double expectedHitRate = 3.0 / 5.0; // 60%
    QCOMPARE(stats.hitRate, expectedHitRate);
    QCOMPARE(cache.getHitRate(), expectedHitRate);
}

void TestCacheManager::testEvictionStatistics()
{
    // 测试驱逐统计
    DatabaseConfig config = createTestConfig(3);
    CacheManager cache(config);
    
    // 填满缓存
    cache.put("key1", QVariant("value1"));
    cache.put("key2", QVariant("value2"));
    cache.put("key3", QVariant("value3"));
    
    CacheStats stats = cache.getStats();
    QCOMPARE(stats.evictionCount, 0);
    
    // 触发驱逐
    cache.put("key4", QVariant("value4"));
    
    stats = cache.getStats();
    QCOMPARE(stats.evictionCount, 1);
    
    // 再次触发驱逐
    cache.put("key5", QVariant("value5"));
    cache.put("key6", QVariant("value6"));
    
    stats = cache.getStats();
    QCOMPARE(stats.evictionCount, 3);
}

void TestCacheManager::testStatisticsReset()
{
    // 测试统计重置
    DatabaseConfig config = createTestConfig(10);
    CacheManager cache(config);
    
    // 执行一些操作
    cache.put("key1", QVariant("value1"));
    cache.get("key1");
    cache.get("key2"); // 未命中
    
    CacheStats stats = cache.getStats();
    QVERIFY(stats.totalRequests > 0);
    QVERIFY(stats.hitCount > 0);
    QVERIFY(stats.missCount > 0);
    
    // 重置统计
    cache.resetStats();
    
    stats = cache.getStats();
    QCOMPARE(stats.totalRequests, 0);
    QCOMPARE(stats.hitCount, 0);
    QCOMPARE(stats.missCount, 0);
    QCOMPARE(stats.hitRate, 0.0);
    QCOMPARE(stats.evictionCount, 0);
    QCOMPARE(stats.expiredCount, 0);
    
    // 验证缓存内容没有被清除
    QVERIFY(cache.contains("key1"));
    QCOMPARE(stats.currentSize, 1);
    QCOMPARE(stats.maxSize, 10);
}

#include "test_cachemanager.moc"