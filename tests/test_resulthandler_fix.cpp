#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QTemporaryFile>
#include <QDir>

#include "QtMyBatisORM/resulthandler.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestResultHandlerFix : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingleColumnQuery();
    void testMultiColumnQuery();
    void testCountQuery();
    void testEmptyResult();

private:
    QSqlDatabase m_db;
    QString m_dbFile;
    ResultHandler* m_handler;
};

void TestResultHandlerFix::initTestCase()
{
    // 创建临时数据库文件
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    m_dbFile = tempFile.fileName();
    tempFile.close();
    
    // 初始化数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE", "test_resulthandler");
    m_db.setDatabaseName(m_dbFile);
    QVERIFY(m_db.open());
    
    // 创建测试表和数据
    QSqlQuery query(m_db);
    QVERIFY(query.exec("CREATE TABLE test_table (id INTEGER, name TEXT, value INTEGER)"));
    QVERIFY(query.exec("INSERT INTO test_table VALUES (1, 'Alice', 100)"));
    QVERIFY(query.exec("INSERT INTO test_table VALUES (2, 'Bob', 200)"));
    QVERIFY(query.exec("INSERT INTO test_table VALUES (3, 'Charlie', 300)"));
    
    // 创建ResultHandler实例
    m_handler = new ResultHandler(this);
}

void TestResultHandlerFix::cleanupTestCase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("test_resulthandler");
    
    // 删除临时文件
    QFile::remove(m_dbFile);
    
    delete m_handler;
}

void TestResultHandlerFix::testSingleColumnQuery()
{
    QSqlQuery query(m_db);
    QVERIFY(query.exec("SELECT COUNT(*) FROM test_table"));
    
    QVariant result = m_handler->handleSingleResult(query);
    
    // 验证返回的是直接的整数值，不是QVariantMap
    QVERIFY(result.isValid());
    QVERIFY(!result.isNull());
    QCOMPARE(result.type(), QVariant::LongLong); // SQLite返回的是LongLong类型
    QCOMPARE(result.toInt(), 3);
    
    qDebug() << "Single column result:" << result;
    qDebug() << "Result type:" << result.typeName();
}

void TestResultHandlerFix::testMultiColumnQuery()
{
    QSqlQuery query(m_db);
    QVERIFY(query.exec("SELECT id, name FROM test_table WHERE id = 1"));
    
    QVariant result = m_handler->handleSingleResult(query);
    
    // 验证返回的是QVariantMap
    QVERIFY(result.isValid());
    QVERIFY(!result.isNull());
    QCOMPARE(result.type(), QVariant::Map);
    
    QVariantMap resultMap = result.toMap();
    QCOMPARE(resultMap["id"].toInt(), 1);
    QCOMPARE(resultMap["name"].toString(), QString("Alice"));
    
    qDebug() << "Multi column result:" << resultMap;
}

void TestResultHandlerFix::testCountQuery()
{
    QSqlQuery query(m_db);
    QVERIFY(query.exec("SELECT COUNT(*) FROM test_table WHERE value > 150"));
    
    QVariant result = m_handler->handleSingleResult(query);
    
    // 验证COUNT查询返回直接的数值
    QVERIFY(result.isValid());
    QCOMPARE(result.toInt(), 2); // Bob和Charlie的value > 150
    
    qDebug() << "Count query result:" << result;
}

void TestResultHandlerFix::testEmptyResult()
{
    QSqlQuery query(m_db);
    QVERIFY(query.exec("SELECT * FROM test_table WHERE id = 999"));
    
    QVariant result = m_handler->handleSingleResult(query);
    
    // 验证空结果
    QVERIFY(!result.isValid() || result.isNull());
    
    qDebug() << "Empty result:" << result;
}

QTEST_MAIN(TestResultHandlerFix)
#include "test_resulthandler_fix.moc" 