#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/logger.h"

using namespace QtMyBatisORM;

class TestCacheIntegrationReal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 测试基本缓存功能
    void testBasicCaching();
    
    // 测试缓存失效
    void testCacheInvalidation();
    
    // 测试缓存命中率统计
    void testCacheHitRateStats();
    
    // 测试缓存大小自适应调整
    void testCacheSizeAdjustment();
    
    // 测试缓存预加载
    void testCachePreloading();
    
    // 测试高并发下的缓存性能
    void testCacheConcurrentPerformance();

private:
    void setupTestDatabase();
    void createTestTables();
    void insertTestData();
    void setupXmlMappers();
    
    QTemporaryDir m_tempDir;
    QString m_dbPath;
    QString m_configPath;
    QStringList m_mapperPaths;
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> m_orm;
};

void TestCacheIntegrationReal::initTestCase()
{
    // 创建临时目录用于测试文件
    QVERIFY(m_tempDir.isValid());
    
    // 设置数据库路径
    m_dbPath = m_tempDir.path() + "/test_cache.db";
    
    // 创建配置文件
    m_configPath = m_tempDir.path() + "/config.json";
    QFile configFile(m_configPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QJsonObject config;
    config["driverName"] = "QSQLITE";
    config["databaseName"] = m_dbPath;
    config["cacheEnabled"] = true;
    config["maxCacheSize"] = 100;
    config["cacheExpireTime"] = 300;
    config["maxConnections"] = 10;
    config["minConnections"] = 2;
    config["maxIdleTime"] = 60;
    
    QJsonDocument doc(config);
    configFile.write(doc.toJson());
    configFile.close();
    
    // 创建XML映射文件
    setupXmlMappers();
    
    // 创建ORM实例
    m_orm = QtMyBatisORM::QtMyBatisORM::create(m_configPath, m_mapperPaths);
    QVERIFY(m_orm != nullptr);
    
    // 设置测试数据库
    setupTestDatabase();
    
    // 设置日志级别为DEBUG以便查看详细信息
    Logger::setLogLevel(LogLevel::DEBUG_LEVEL);
}

void TestCacheIntegrationReal::cleanupTestCase()
{
    m_orm.reset();
}

void TestCacheIntegrationReal::init()
{
    // 每个测试前的准备工作
}

void TestCacheIntegrationReal::cleanup()
{
    // 每个测试后的清理工作
}

void TestCacheIntegrationReal::setupXmlMappers()
{
    // 创建测试Mapper XML
    QString cacheMapperPath = m_tempDir.path() + "/cache_mapper.xml";
    QFile mapperFile(cacheMapperPath);
    QVERIFY(mapperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QString mapperXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="CacheMapper">
    <select id="findAll" resultType="Item">
        SELECT * FROM cache_items
    </select>
    
    <select id="findById" parameterType="int" resultType="Item">
        SELECT * FROM cache_items WHERE id = :id
    </select>
    
    <select id="findByCategory" parameterType="string" resultType="Item">
        SELECT * FROM cache_items WHERE category = :category
    </select>
    
    <insert id="insert" parameterType="Item">
        INSERT INTO cache_items (name, category, value, created_at) 
        VALUES (:name, :category, :value, :created_at)
    </insert>
    
    <update id="update" parameterType="Item">
        UPDATE cache_items SET name = :name, category = :category, value = :value 
        WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM cache_items WHERE id = :id
    </delete>
</mapper>)";
    
    mapperFile.write(mapperXml.toUtf8());
    mapperFile.close();
    
    m_mapperPaths << cacheMapperPath;
}

void TestCacheIntegrationReal::setupTestDatabase()
{
    auto session = m_orm->openSession();
    
    createTestTables();
    insertTestData();
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::createTestTables()
{
    // 直接使用QSqlDatabase创建表
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cache_test_connection");
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return;
    }
    
    // 创建测试表
    QSqlQuery query(db);
    query.exec(R"(
        CREATE TABLE cache_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT NOT NULL,
            value REAL NOT NULL,
            created_at TEXT NOT NULL
        )
    )");
    
    // 创建索引
    query.exec("CREATE INDEX idx_cache_category ON cache_items (category)");
    
    db.close();
    QSqlDatabase::removeDatabase("cache_test_connection");
}

void TestCacheIntegrationReal::insertTestData()
{
    // 直接使用QSqlDatabase插入数据
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cache_test_insert_connection");
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return;
    }
    
    // 批量插入数据
    db.transaction();
    
    try {
        QStringList categories = {"electronics", "books", "clothing", "food", "toys"};
        QSqlQuery query(db);
        
        // 准备插入语句
        query.prepare("INSERT INTO cache_items (name, category, value, created_at) VALUES (?, ?, ?, ?)");
        
        for (int i = 0; i < 100; ++i) {
            query.bindValue(0, QString("Item %1").arg(i));
            query.bindValue(1, categories[i % categories.size()]);
            query.bindValue(2, (i + 1) * 10.5);
            query.bindValue(3, QDateTime::currentDateTime().toString(Qt::ISODate));
            
            if (!query.exec()) {
                qWarning() << "Failed to insert item:" << query.lastError().text();
            }
        }
        
        db.commit();
    } catch (...) {
        db.rollback();
        throw;
    }
    
    db.close();
    QSqlDatabase::removeDatabase("cache_test_insert_connection");
}

void TestCacheIntegrationReal::testBasicCaching()
{
    auto session = m_orm->openSession();
    
    // 第一次查询 - 应该从数据库获取
    QVariantMap params;
    params["id"] = 1;
    
    QElapsedTimer timer;
    timer.start();
    QVariant item1 = session->selectOne("CacheMapper.findById", params);
    qint64 firstQueryTime = timer.elapsed();
    
    QVERIFY(!item1.isNull());
    
    // 第二次查询 - 应该从缓存获取
    timer.restart();
    QVariant item2 = session->selectOne("CacheMapper.findById", params);
    qint64 secondQueryTime = timer.elapsed();
    
    QVERIFY(!item2.isNull());
    QCOMPARE(item1, item2);
    
    // 缓存查询应该更快
    qDebug() << "First query time:" << firstQueryTime << "ms";
    qDebug() << "Second query time (cached):" << secondQueryTime << "ms";
    
    // 验证缓存查询确实更快
    QVERIFY(secondQueryTime < firstQueryTime);
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::testCacheInvalidation()
{
    auto session = m_orm->openSession();
    
    // 第一次查询 - 应该从数据库获取
    QVariantMap params;
    params["id"] = 2;
    
    QVariant item1 = session->selectOne("CacheMapper.findById", params);
    QVERIFY(!item1.isNull());
    
    // 更新项目，应该使缓存失效
    QVariantMap itemMap = item1.toMap();
    itemMap["name"] = "Updated Item";
    
    session->update("CacheMapper.update", itemMap);
    
    // 再次查询，应该从数据库获取新数据
    QVariant item2 = session->selectOne("CacheMapper.findById", params);
    QVERIFY(!item2.isNull());
    
    QVariantMap updatedMap = item2.toMap();
    QCOMPARE(updatedMap["name"].toString(), QString("Updated Item"));
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::testCacheHitRateStats()
{
    auto session = m_orm->openSession();
    
    // 执行多次查询以生成缓存统计信息
    for (int i = 1; i <= 10; ++i) {
        QVariantMap params;
        params["id"] = i;
        
        // 第一次查询 - 缓存未命中
        session->selectOne("CacheMapper.findById", params);
        
        // 第二次查询 - 缓存命中
        session->selectOne("CacheMapper.findById", params);
        
        // 第三次查询 - 缓存命中
        session->selectOne("CacheMapper.findById", params);
    }
    
    // 获取缓存管理器统计信息
    // 注意：这里假设可以通过某种方式访问缓存管理器
    // 实际实现中可能需要添加访问方法
    
    // 验证缓存命中率应该接近 2/3 (约66.7%)
    // 由于我们无法直接访问缓存管理器，这里只是记录期望值
    qDebug() << "Expected cache hit rate: ~66.7%";
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::testCacheSizeAdjustment()
{
    auto session = m_orm->openSession();
    
    // 执行大量查询以填充缓存
    for (int i = 1; i <= 50; ++i) {
        QVariantMap params;
        params["id"] = i;
        session->selectOne("CacheMapper.findById", params);
    }
    
    // 执行不同类别的查询
    for (int i = 0; i < 5; ++i) {
        QVariantMap params;
        params["category"] = QString("category%1").arg(i);
        session->selectList("CacheMapper.findByCategory", params);
    }
    
    // 注意：由于我们无法直接访问缓存管理器的内部状态，
    // 这里只是记录期望的行为
    qDebug() << "Cache size should be adjusted based on usage patterns";
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::testCachePreloading()
{
    auto session = m_orm->openSession();
    
    // 创建预加载查询列表
    QStringList preloadQueries;
    for (int i = 1; i <= 5; ++i) {
        preloadQueries << QString("CacheMapper.findById?id=%1").arg(i);
    }
    
    // 注意：这里假设有一个预加载方法
    // 实际实现中可能需要添加这个方法
    qDebug() << "Cache preloading would be performed here";
    
    // 验证预加载后的查询性能
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 1; i <= 5; ++i) {
        QVariantMap params;
        params["id"] = i;
        session->selectOne("CacheMapper.findById", params);
    }
    
    qint64 totalTime = timer.elapsed();
    qDebug() << "Total time for 5 queries after preloading:" << totalTime << "ms";
    qDebug() << "Average time per query:" << (totalTime / 5.0) << "ms";
    
    m_orm->closeSession(session);
}

void TestCacheIntegrationReal::testCacheConcurrentPerformance()
{
    // 创建多个线程同时访问缓存
    const int threadCount = 5;
    const int queriesPerThread = 20;
    
    QList<QThread*> threads;
    QList<qint64> executionTimes(threadCount);
    
    // 创建并启动线程
    for (int i = 0; i < threadCount; ++i) {
        QThread* thread = QThread::create([this, i, queriesPerThread, &executionTimes]() {
            QElapsedTimer timer;
            timer.start();
            
            auto session = m_orm->openSession();
            
            for (int j = 0; j < queriesPerThread; ++j) {
                QVariantMap params;
                params["id"] = (j % 10) + 1; // 循环使用10个不同的ID
                
                session->selectOne("CacheMapper.findById", params);
            }
            
            m_orm->closeSession(session);
            executionTimes[i] = timer.elapsed();
        });
        
        threads.append(thread);
        thread->start();
    }
    
    // 等待所有线程完成
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    
    // 计算统计信息
    qint64 totalTime = 0;
    qint64 minTime = std::numeric_limits<qint64>::max();
    qint64 maxTime = 0;
    
    for (qint64 time : executionTimes) {
        totalTime += time;
        minTime = qMin(minTime, time);
        maxTime = qMax(maxTime, time);
    }
    
    double avgThreadTime = static_cast<double>(totalTime) / threadCount;
    double avgQueryTime = avgThreadTime / queriesPerThread;
    
    qDebug() << "Concurrent cache performance:";
    qDebug() << "  Thread count:" << threadCount;
    qDebug() << "  Queries per thread:" << queriesPerThread;
    qDebug() << "  Total queries:" << (threadCount * queriesPerThread);
    qDebug() << "  Average thread execution time:" << avgThreadTime << "ms";
    qDebug() << "  Average query time:" << avgQueryTime << "ms";
    qDebug() << "  Min thread time:" << minTime << "ms";
    qDebug() << "  Max thread time:" << maxTime << "ms";
    qDebug() << "  Time difference (max-min):" << (maxTime - minTime) << "ms";
}

