#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QDateTime>

#include "QtMyBatisORM/logger.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/objectpool.h"

using namespace QtMyBatisORM;

class TestPerformanceBenchmark : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 测试日志性能
    void testLoggerPerformance();
    
    // 测试缓存键生成性能
    void testCacheKeyGeneration();
    
    // 测试SQL处理性能
    void testSqlProcessing();
    
    // 测试对象池性能
    void testObjectPoolPerformance();
    
    // 测试批量操作性能
    void testBatchOperations();

private:
    QSharedPointer<QSqlDatabase> m_db;
    QSharedPointer<Executor> m_executor;
    QSharedPointer<CacheManager> m_cacheManager;
    
    // 辅助方法
    void setupDatabase();
    void createTestTable();
    void dropTestTable();
    QVariantMap createRandomRecord(int id);
    
    // 性能测试辅助方法
    qint64 measureExecutionTime(std::function<void()> func, int iterations = 1);
    void reportPerformance(const QString& testName, qint64 elapsedMs, int iterations);
};

void TestPerformanceBenchmark::initTestCase()
{
    // 设置日志级别为ERROR，减少测试输出
    Logger::setLogLevel(LogLevel::ERROR);
    
    // 设置测试数据库
    setupDatabase();
    
    // 创建测试表
    createTestTable();
    
    // 创建缓存管理器
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    m_cacheManager = QSharedPointer<CacheManager>::create(config);
    
    // 创建执行器
    m_executor = QSharedPointer<Executor>::create(m_db, m_cacheManager);
}

void TestPerformanceBenchmark::cleanupTestCase()
{
    // 清理测试表
    dropTestTable();
    
    // 关闭数据库连接
    if (m_db && m_db->isOpen()) {
        m_db->close();
    }
}

void TestPerformanceBenchmark::init()
{
    // 每个测试前的准备工作
}

void TestPerformanceBenchmark::cleanup()
{
    // 每个测试后的清理工作
    if (m_cacheManager) {
        m_cacheManager->clear();
    }
}

void TestPerformanceBenchmark::testLoggerPerformance()
{
    // 测试不同日志级别的性能
    Logger::setLogLevel(LogLevel::INFO);
    
    // 测试简单日志
    qint64 simpleLogTime = measureExecutionTime([&]() {
        Logger::info("This is a simple log message");
    }, 10000);
    reportPerformance("Simple logging", simpleLogTime, 10000);
    
    // 测试带上下文的日志
    QVariantMap context;
    context["userId"] = 12345;
    context["action"] = "login";
    context["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    qint64 contextLogTime = measureExecutionTime([&]() {
        Logger::info("User login action", context);
    }, 10000);
    reportPerformance("Logging with context", contextLogTime, 10000);
    
    // 测试过滤的日志 (不应该有性能开销)
    Logger::setLogLevel(LogLevel::ERROR);
    qint64 filteredLogTime = measureExecutionTime([&]() {
        Logger::debug("This debug message should be filtered");
    }, 10000);
    reportPerformance("Filtered logging", filteredLogTime, 10000);
    
    // 恢复日志级别
    Logger::setLogLevel(LogLevel::ERROR);
}

void TestPerformanceBenchmark::testCacheKeyGeneration()
{
    // 准备测试数据
    QString statementId = "user.findById";
    QVariantMap parameters;
    parameters["id"] = 12345;
    parameters["name"] = "John Doe";
    parameters["active"] = true;
    parameters["createdAt"] = QDateTime::currentDateTime();
    parameters["score"] = 95.5;
    
    // 测试缓存键生成性能
    qint64 cacheKeyTime = measureExecutionTime([&]() {
        m_executor->generateCacheKey(statementId, parameters);
    }, 10000);
    reportPerformance("Cache key generation", cacheKeyTime, 10000);
}

void TestPerformanceBenchmark::testSqlProcessing()
{
    // 准备测试数据
    QString sql = "SELECT * FROM test_table WHERE id = #{id} AND name LIKE #{name} AND active = #{active}";
    QVariantMap parameters;
    parameters["id"] = 12345;
    parameters["name"] = "%John%";
    parameters["active"] = true;
    
    // 测试SQL处理性能
    qint64 sqlProcessingTime = measureExecutionTime([&]() {
        m_executor->query(sql, parameters);
    }, 1000);
    reportPerformance("SQL processing", sqlProcessingTime, 1000);
    
    // 测试缓存查询性能
    m_executor->queryWithCache("test.query", sql, parameters);
    qint64 cachedQueryTime = measureExecutionTime([&]() {
        m_executor->queryWithCache("test.query", sql, parameters);
    }, 1000);
    reportPerformance("Cached query", cachedQueryTime, 1000);
}

void TestPerformanceBenchmark::testObjectPoolPerformance()
{
    // 创建测试对象池
    ObjectPool<QObject> objectPool(10, 100);
    
    // 测试对象获取和释放性能
    qint64 objectPoolTime = measureExecutionTime([&]() {
        QObject* obj = objectPool.acquire();
        objectPool.release(obj);
    }, 10000);
    reportPerformance("Object pool operations", objectPoolTime, 10000);
    
    // 测试直接创建和销毁对象的性能
    qint64 directCreationTime = measureExecutionTime([&]() {
        QObject* obj = new QObject();
        delete obj;
    }, 10000);
    reportPerformance("Direct object creation", directCreationTime, 10000);
    
    // 比较性能差异
    double improvement = 100.0 * (directCreationTime - objectPoolTime) / directCreationTime;
    qDebug() << "Object pooling improvement:" << QString::number(improvement, 'f', 2) << "%";
}

void TestPerformanceBenchmark::testBatchOperations()
{
    // 准备批量插入数据
    QList<QVariantMap> records;
    for (int i = 1; i <= 100; i++) {
        records.append(createRandomRecord(i));
    }
    
    // 测试单条插入性能
    qint64 singleInsertTime = measureExecutionTime([&]() {
        for (const QVariantMap& record : records) {
            QString sql = "INSERT INTO test_table (id, name, value, created_at) "
                         "VALUES (#{id}, #{name}, #{value}, #{created_at})";
            m_executor->update(sql, record);
        }
    });
    reportPerformance("100 individual inserts", singleInsertTime, 1);
    
    // 清理表
    m_executor->update("DELETE FROM test_table", {});
    
    // 测试批量插入性能
    QString sql = "INSERT INTO test_table (id, name, value, created_at) "
                 "VALUES (#{id}, #{name}, #{value}, #{created_at})";
    
    // 模拟Session的批量插入
    qint64 batchInsertTime = measureExecutionTime([&]() {
        // 开始事务
        m_db->transaction();
        
        try {
            for (const QVariantMap& record : records) {
                m_executor->update(sql, record);
            }
            
            // 提交事务
            m_db->commit();
        } catch (...) {
            m_db->rollback();
            throw;
        }
    });
    reportPerformance("Batch insert of 100 records", batchInsertTime, 1);
    
    // 比较性能差异
    double improvement = 100.0 * (singleInsertTime - batchInsertTime) / singleInsertTime;
    qDebug() << "Batch insert improvement:" << QString::number(improvement, 'f', 2) << "%";
}

// 辅助方法实现
void TestPerformanceBenchmark::setupDatabase()
{
    // 创建内存数据库用于测试
    m_db = QSharedPointer<QSqlDatabase>::create(QSqlDatabase::addDatabase("QSQLITE", "performance_test"));
    m_db->setDatabaseName(":memory:");
    
    if (!m_db->open()) {
        QFAIL(QString("Failed to open database: %1").arg(m_db->lastError().text()).toUtf8().constData());
    }
}

void TestPerformanceBenchmark::createTestTable()
{
    QSqlQuery query(*m_db);
    if (!query.exec("CREATE TABLE test_table ("
                   "id INTEGER PRIMARY KEY, "
                   "name TEXT, "
                   "value REAL, "
                   "created_at TEXT)")) {
        QFAIL(QString("Failed to create test table: %1").arg(query.lastError().text()).toUtf8().constData());
    }
}

void TestPerformanceBenchmark::dropTestTable()
{
    QSqlQuery query(*m_db);
    query.exec("DROP TABLE IF EXISTS test_table");
}

QVariantMap TestPerformanceBenchmark::createRandomRecord(int id)
{
    QVariantMap record;
    record["id"] = id;
    record["name"] = QString("Test Record %1").arg(id);
    record["value"] = id * 1.5;
    record["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return record;
}

qint64 TestPerformanceBenchmark::measureExecutionTime(std::function<void()> func, int iterations)
{
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < iterations; i++) {
        func();
    }
    
    return timer.elapsed();
}

void TestPerformanceBenchmark::reportPerformance(const QString& testName, qint64 elapsedMs, int iterations)
{
    double avgTimeMs = static_cast<double>(elapsedMs) / iterations;
    qDebug() << "Performance -" << testName << ":" 
             << QString::number(avgTimeMs, 'f', 3) << "ms per operation"
             << "(" << iterations << "iterations in" << elapsedMs << "ms)";
}

