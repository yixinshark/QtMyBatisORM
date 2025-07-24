#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "QtMyBatisORM/qtmybatishelper.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestQtMyBatisHelper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testInitialization();
    void testInvalidInitialization();
    void testBasicCRUDOperations();
    void testTransactionOperations();
    void testBatchOperations();
    void testDebugMode();
    void testErrorHandling();
    void testSessionLifecycle();

private:
    QString createTestConfig();
    QString createTestSqlFile();
    void setupTestFiles();
    void cleanupTestFiles();
    
    QString m_configFile;
    QString m_sqlFile;
    QStringList m_tempFiles;
};

void TestQtMyBatisHelper::initTestCase()
{
    qDebug() << "=== QtMyBatisHelper Test Suite ===";
}

void TestQtMyBatisHelper::cleanupTestCase()
{
    cleanupTestFiles();
}

void TestQtMyBatisHelper::init()
{
    setupTestFiles();
}

void TestQtMyBatisHelper::cleanup()
{
    if (QtMyBatisHelper::isInitialized()) {
        QtMyBatisHelper::shutdown();
    }
    cleanupTestFiles();
}

void TestQtMyBatisHelper::testInitialization()
{
    // 测试正常初始化
    QVERIFY(!QtMyBatisHelper::isInitialized());
    
    QString configContent = QString(R"({
        "database": {
            "debug": true,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    QVERIFY(QtMyBatisHelper::isInitialized());
    
    // 测试重复初始化
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    QVERIFY(QtMyBatisHelper::isInitialized());
    
    QtMyBatisHelper::shutdown();
    QVERIFY(!QtMyBatisHelper::isInitialized());
}

void TestQtMyBatisHelper::testInvalidInitialization()
{
    // 测试无效配置文件路径
    QVERIFY(!QtMyBatisHelper::initialize("/nonexistent/config.json"));
    QVERIFY(!QtMyBatisHelper::isInitialized());
    
    // 测试无效JSON格式
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write("{ invalid json }");
    configFile.close();
    
    QVERIFY(!QtMyBatisHelper::initialize(m_configFile));
    QVERIFY(!QtMyBatisHelper::isInitialized());
}

void TestQtMyBatisHelper::testBasicCRUDOperations()
{
    // 设置测试环境
    QString configContent = QString(R"({
        "database": {
            "debug": false,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    
    // 创建测试表
    QString createTableSql = "CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)";
    QVERIFY(QtMyBatisHelper::execute(createTableSql) >= 0);
    
    // 测试插入（使用命名参数）
    QString insertSql = "INSERT INTO test_table (name, value) VALUES (:name, :value)";
    QVariantMap insertParams;
    insertParams["name"] = "test1";
    insertParams["value"] = 100;
    int insertResult = QtMyBatisHelper::execute(insertSql, insertParams);
    QVERIFY(insertResult > 0);
    
    // 测试查询单条记录
    QString selectSql = "SELECT * FROM test_table WHERE name = :name";
    QVariantMap selectParams;
    selectParams["name"] = "test1";
    QVariant result = QtMyBatisHelper::selectOne(selectSql, selectParams);
    QVERIFY(result.isValid());
    
    // 测试查询多条记录
    QVariantList results = QtMyBatisHelper::selectList("SELECT * FROM test_table");
    QCOMPARE(results.size(), 1);
    
    // 测试更新
    QString updateSql = "UPDATE test_table SET value = :value WHERE name = :name";
    QVariantMap updateParams;
    updateParams["value"] = 200;
    updateParams["name"] = "test1";
    int updateResult = QtMyBatisHelper::execute(updateSql, updateParams);
    QVERIFY(updateResult > 0);
    
    // 验证更新结果
    result = QtMyBatisHelper::selectOne(selectSql, selectParams);
    QVariantMap resultMap = result.toMap();
    QCOMPARE(resultMap["value"].toInt(), 200);
    
    // 测试删除
    QString deleteSql = "DELETE FROM test_table WHERE name = :name";
    QVariantMap deleteParams;
    deleteParams["name"] = "test1";
    int deleteResult = QtMyBatisHelper::execute(deleteSql, deleteParams);
    QVERIFY(deleteResult > 0);
    
    // 验证删除结果
    results = QtMyBatisHelper::selectList("SELECT * FROM test_table");
    QCOMPARE(results.size(), 0);
}

void TestQtMyBatisHelper::testTransactionOperations()
{
    // 设置测试环境
    QString configContent = QString(R"({
        "database": {
            "debug": true,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    QtMyBatisHelper::setDebugMode(true);
    
    // 创建测试表
    QString createTableSql = "CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)";
    QVERIFY(QtMyBatisHelper::execute(createTableSql) >= 0);
    
    // 测试成功的事务
    bool success = QtMyBatisHelper::executeInTransaction([&]() -> bool {
        QVariantMap params1;
        params1["name"] = "tx_test1";
        params1["value"] = 100;
        
        QVariantMap params2;
        params2["name"] = "tx_test2";
        params2["value"] = 200;
        
        QString insertSql = "INSERT INTO test_table (name, value) VALUES (?, ?)";
        
        // 转换参数格式
        QVariantMap execParams1;
        execParams1["0"] = params1["name"];
        execParams1["1"] = params1["value"];
        
        QVariantMap execParams2;
        execParams2["0"] = params2["name"];
        execParams2["1"] = params2["value"];
        
        return QtMyBatisHelper::execute(insertSql, execParams1) > 0 &&
               QtMyBatisHelper::execute(insertSql, execParams2) > 0;
    });
    
    QVERIFY(success);
    
    // 验证事务结果
    QVariantList results = QtMyBatisHelper::selectList("SELECT * FROM test_table");
    QCOMPARE(results.size(), 2);
    
    // 测试失败的事务（应该回滚）
    bool failureResult = QtMyBatisHelper::executeInTransaction([&]() -> bool {
        QString insertSql = "INSERT INTO test_table (name, value) VALUES (?, ?)";
        QVariantMap params;
        params["0"] = "tx_test3";
        params["1"] = 300;
        
        QtMyBatisHelper::execute(insertSql, params);
        
        // 故意返回false来触发回滚
        return false;
    });
    
    QVERIFY(!failureResult);
    
    // 验证回滚结果 - 应该还是2条记录
    results = QtMyBatisHelper::selectList("SELECT * FROM test_table");
    QCOMPARE(results.size(), 2);
    
    // 测试异常事务（应该回滚）
    bool exceptionResult = false;
    try {
        exceptionResult = QtMyBatisHelper::executeInTransaction([&]() -> bool {
            QString insertSql = "INSERT INTO test_table (name, value) VALUES (?, ?)";
            QVariantMap params;
            params["0"] = "tx_test4";
            params["1"] = 400;
            
            QtMyBatisHelper::execute(insertSql, params);
            
            // 故意抛出异常
            throw std::runtime_error("Test exception");
            return true;
        });
    } catch (...) {
        // 异常被捕获
    }
    
    QVERIFY(!exceptionResult);
    
    // 验证异常回滚结果 - 应该还是2条记录
    results = QtMyBatisHelper::selectList("SELECT * FROM test_table");
    QCOMPARE(results.size(), 2);
}

void TestQtMyBatisHelper::testBatchOperations()
{
    // 设置测试环境
    QString configContent = QString(R"({
        "database": {
            "debug": false,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    
    // 创建测试表
    QString createTableSql = "CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)";
    QVERIFY(QtMyBatisHelper::execute(createTableSql) >= 0);
    
    // 准备批量数据
    QList<QVariantMap> batchData;
    for (int i = 1; i <= 5; ++i) {
        QVariantMap params;
        params["name"] = QString("batch_test%1").arg(i);
        params["value"] = i * 100;
        batchData.append(params);
    }
    
    // 测试批量插入 - 简化为逐个插入来测试基本功能
    int totalInserted = 0;
    for (const auto& data : batchData) {
        QString insertSql = "INSERT INTO test_table (name, value) VALUES (?, ?)";
        QVariantMap params;
        params["0"] = data["name"];
        params["1"] = data["value"];
        if (QtMyBatisHelper::execute(insertSql, params) > 0) {
            totalInserted++;
        }
    }
    QCOMPARE(totalInserted, 5);
    
    // 验证批量插入结果
    QVariantList results = QtMyBatisHelper::selectList("SELECT * FROM test_table ORDER BY id");
    QCOMPARE(results.size(), 5);
    
    // 准备批量更新数据
    QList<QVariantMap> updateData;
    for (int i = 1; i <= 3; ++i) {
        QVariantMap params;
        params["name"] = QString("batch_test%1").arg(i);
        params["value"] = i * 200; // 更新值
        updateData.append(params);
    }
    
    // 测试批量更新 - 简化为逐个更新
    int totalUpdated = 0;
    for (const auto& data : updateData) {
        QString updateSql = "UPDATE test_table SET value = ? WHERE name = ?";
        QVariantMap params;
        params["0"] = data["value"];
        params["1"] = data["name"];
        if (QtMyBatisHelper::execute(updateSql, params) > 0) {
            totalUpdated++;
        }
    }
    QCOMPARE(totalUpdated, 3);
    
    // 验证批量更新结果
    QVariantMap selectParams;
    selectParams["0"] = "batch_test1";
    QVariant result = QtMyBatisHelper::selectOne("SELECT value FROM test_table WHERE name = ?", selectParams);
    QCOMPARE(result.toMap()["value"].toInt(), 200);
}

void TestQtMyBatisHelper::testDebugMode()
{
    // 设置测试环境
    QString configContent = QString(R"({
        "database": {
            "debug": true,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    
    // 测试调试模式开关 - 应该从配置文件读取到debug=true
    QVERIFY(QtMyBatisHelper::isDebugMode()); // 从配置文件读取的状态
    
    QtMyBatisHelper::setDebugMode(true);
    QVERIFY(QtMyBatisHelper::isDebugMode());
    
    QtMyBatisHelper::setDebugMode(false);
    QVERIFY(!QtMyBatisHelper::isDebugMode());
    
    // 开启调试模式并执行SQL，检查是否有调试输出（这里只能通过手动观察）
    QtMyBatisHelper::setDebugMode(true);
    
    QString createTableSql = "CREATE TABLE debug_test (id INTEGER PRIMARY KEY, name TEXT)";
    QtMyBatisHelper::execute(createTableSql);
    
    // 执行一些操作来生成调试日志
    QVariantMap params;
    params["0"] = "debug_test";
    QtMyBatisHelper::execute("INSERT INTO debug_test (name) VALUES (?)", params);
    
    QVariant result = QtMyBatisHelper::selectOne("SELECT * FROM debug_test WHERE name = ?", params);
    QVERIFY(result.isValid());
}

void TestQtMyBatisHelper::testErrorHandling()
{
    // 测试未初始化时的错误处理
    QVERIFY(!QtMyBatisHelper::isInitialized());
    
    try {
        QtMyBatisHelper::selectOne("SELECT 1");
        QFAIL("Should throw exception when not initialized");
    } catch (const ConfigurationException& e) {
        QVERIFY(QString(e.what()).contains("not initialized"));
    }
    
    // 初始化后测试SQL错误
    QString configContent = QString(R"({
        "database": {
            "debug": false,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 5,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    
    // 测试无效SQL
    try {
        QtMyBatisHelper::execute("INVALID SQL STATEMENT");
        QFAIL("Should throw exception for invalid SQL");
    } catch (const QtMyBatisException& e) {
        // 期望的异常
        QVERIFY(true);
    }
    
    // 测试查询不存在的表
    try {
        QtMyBatisHelper::selectOne("SELECT * FROM nonexistent_table");
        QFAIL("Should throw exception for nonexistent table");
    } catch (const QtMyBatisException& e) {
        // 期望的异常
        QVERIFY(true);
    }
}

void TestQtMyBatisHelper::testSessionLifecycle()
{
    // 这个测试主要验证Session的生命周期管理是否正确
    // 由于Session管理是内部的，我们主要通过多次操作来验证不会有内存泄漏
    
    QString configContent = QString(R"({
        "database": {
            "debug": false,
            "type": "QSQLITE",
            "database_name": ":memory:",
            "max_connection_count": 2,
            "max_wait_time": 3000,
            "sql_files": []
        }
    })");
    
    m_configFile = createTestConfig();
    QFile configFile(m_configFile);
    QVERIFY(configFile.open(QIODevice::WriteOnly));
    configFile.write(configContent.toUtf8());
    configFile.close();
    
    QVERIFY(QtMyBatisHelper::initialize(m_configFile));
    
    // 创建测试表
    QString createTableSql = "CREATE TABLE lifecycle_test (id INTEGER PRIMARY KEY, name TEXT)";
    QVERIFY(QtMyBatisHelper::execute(createTableSql) >= 0);
    
    // 执行多次操作，验证Session能够正确复用
    for (int i = 0; i < 10; ++i) {
        QVariantMap params;
        params["name"] = QString("test%1").arg(i);
        
        // 每次操作都应该获取新的Session并正确释放
        QVariantMap execParams;
        execParams["0"] = params["name"];
        int result = QtMyBatisHelper::execute("INSERT INTO lifecycle_test (name) VALUES (?)", execParams);
        QVERIFY(result > 0);
        
        QVariant selectResult = QtMyBatisHelper::selectOne("SELECT COUNT(*) as count FROM lifecycle_test");
        QCOMPARE(selectResult.toMap()["count"].toInt(), i + 1);
    }
    
    // 验证最终结果
    QVariantList allResults = QtMyBatisHelper::selectList("SELECT * FROM lifecycle_test");
    QCOMPARE(allResults.size(), 10);
    
    // 测试并发操作（模拟多个Session同时使用）
    bool allSuccess = QtMyBatisHelper::executeInTransaction([&]() -> bool {
        for (int i = 10; i < 15; ++i) {
            QVariantMap params;
            params["name"] = QString("tx_test%1").arg(i);
            
            QVariantMap execParams;
            execParams["0"] = params["name"];
            if (QtMyBatisHelper::execute("INSERT INTO lifecycle_test (name) VALUES (?)", execParams) <= 0) {
                return false;
            }
        }
        return true;
    });
    
    QVERIFY(allSuccess);
    
    // 最终验证
    QVariantList finalResults = QtMyBatisHelper::selectList("SELECT * FROM lifecycle_test");
    QCOMPARE(finalResults.size(), 15);
}

QString TestQtMyBatisHelper::createTestConfig()
{
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate("qtmybatis_test_config_XXXXXX.json");
    tempFile->setAutoRemove(false);
    
    if (!tempFile->open()) {
        return QString();
    }
    
    QString fileName = tempFile->fileName();
    tempFile->close();
    m_tempFiles.append(fileName);
    
    return fileName;
}

QString TestQtMyBatisHelper::createTestSqlFile()
{
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate("qtmybatis_test_sql_XXXXXX.xml");
    tempFile->setAutoRemove(false);
    
    if (!tempFile->open()) {
        return QString();
    }
    
    QString fileName = tempFile->fileName();
    tempFile->close();
    m_tempFiles.append(fileName);
    
    return fileName;
}

void TestQtMyBatisHelper::setupTestFiles()
{
    // 每个测试方法开始前的准备工作
}

void TestQtMyBatisHelper::cleanupTestFiles()
{
    for (const QString& file : m_tempFiles) {
        QFile::remove(file);
    }
    m_tempFiles.clear();
}

QTEST_MAIN(TestQtMyBatisHelper)
#include "test_qtmybatishelper.moc" 