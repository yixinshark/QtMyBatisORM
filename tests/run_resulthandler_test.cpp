#include <QtTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "QtMyBatisORM/resulthandler.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestResultHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testConstructor();
    void testHandleSingleResult();
    void testHandleSingleResultEmpty();
    void testHandleListResult();
    void testHandleListResultEmpty();
    void testRecordToMap();
    void testConvertFromSqlType();
    void testConvertFromSqlTypeWithTargetTypes();
    void testConvertFromSqlTypeJson();
    void testConvertFromSqlTypeArray();
    void testGetColumnNames();
    void testErrorHandling();
    void testNullValues();
    void testComplexDataTypes();

private:
    void setupTestDatabase();
    void createTestTable();
    void insertTestData();
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<ResultHandler> m_handler;
    QString m_connectionName;
};

void TestResultHandler::initTestCase()
{
    m_connectionName = "test_resulthandler_connection";
    setupTestDatabase();
}

void TestResultHandler::cleanupTestCase()
{
    if (m_connection && m_connection->isOpen()) {
        m_connection->close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void TestResultHandler::init()
{
    m_handler = QSharedPointer<ResultHandler>::create();
    if (m_connection && m_connection->isOpen()) {
        createTestTable();
        insertTestData();
    }
}

void TestResultHandler::cleanup()
{
    if (m_connection && m_connection->isOpen()) {
        QSqlQuery query(*m_connection);
        query.exec("DROP TABLE IF EXISTS test_data");
    }
    m_handler.reset();
}

void TestResultHandler::setupTestDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        QFAIL("Failed to open test database");
    }
    
    m_connection = QSharedPointer<QSqlDatabase>::create(db);
}

void TestResultHandler::createTestTable()
{
    QSqlQuery query(*m_connection);
    QString createTableSql = R"(
        CREATE TABLE test_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            age INTEGER,
            salary REAL,
            active BOOLEAN,
            birth_date DATE,
            created_at DATETIME,
            json_data TEXT,
            binary_data BLOB
        )
    )";
    
    if (!query.exec(createTableSql)) {
        QFAIL(QString("Failed to create test table: %1").arg(query.lastError().text()).toLocal8Bit());
    }
}

void TestResultHandler::insertTestData()
{
    QSqlQuery query(*m_connection);
    
    // 插入测试数据
    query.exec("INSERT INTO test_data (name, age, salary, active, birth_date, created_at, json_data) VALUES "
               "('Alice', 25, 50000.50, 1, '1998-05-15', '2023-01-01 10:30:00', '{\"skills\": [\"C++\", \"Qt\"]}')");
    
    query.exec("INSERT INTO test_data (name, age, salary, active, birth_date, created_at, json_data) VALUES "
               "('Bob', 30, 60000.75, 0, '1993-08-22', '2023-01-02 11:45:00', '{\"department\": \"Engineering\"}')");
    
    query.exec("INSERT INTO test_data (name, age, salary, active, birth_date, created_at, json_data) VALUES "
               "('Charlie', NULL, NULL, NULL, NULL, NULL, NULL)");  // 测试NULL值
}

void TestResultHandler::testConstructor()
{
    QVERIFY(m_handler != nullptr);
}

void TestResultHandler::testHandleSingleResult()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT * FROM test_data WHERE name = 'Alice'");
    
    QVariant result = m_handler->handleSingleResult(query);
    QVERIFY(!result.isNull());
    
    QVariantMap record = result.toMap();
    QCOMPARE(record["name"].toString(), QString("Alice"));
    QCOMPARE(record["age"].toInt(), 25);
    QCOMPARE(record["salary"].toDouble(), 50000.50);
    QCOMPARE(record["active"].toBool(), true);
}

void TestResultHandler::testHandleSingleResultEmpty()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT * FROM test_data WHERE name = 'NonExistent'");
    
    QVariant result = m_handler->handleSingleResult(query);
    QVERIFY(result.isNull());
}

void TestResultHandler::testHandleListResult()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT * FROM test_data WHERE age IS NOT NULL ORDER BY age");
    
    QVariantList results = m_handler->handleListResult(query);
    QCOMPARE(results.size(), 2);
    
    QVariantMap firstRecord = results[0].toMap();
    QCOMPARE(firstRecord["name"].toString(), QString("Alice"));
    QCOMPARE(firstRecord["age"].toInt(), 25);
    
    QVariantMap secondRecord = results[1].toMap();
    QCOMPARE(secondRecord["name"].toString(), QString("Bob"));
    QCOMPARE(secondRecord["age"].toInt(), 30);
}

void TestResultHandler::testHandleListResultEmpty()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT * FROM test_data WHERE name = 'NonExistent'");
    
    QVariantList results = m_handler->handleListResult(query);
    QVERIFY(results.isEmpty());
}

void TestResultHandler::testRecordToMap()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT name, age, salary FROM test_data WHERE name = 'Alice'");
    
    if (query.next()) {
        QVariantMap record = m_handler->recordToMap(query);
        
        QVERIFY(record.contains("name"));
        QVERIFY(record.contains("age"));
        QVERIFY(record.contains("salary"));
        
        QCOMPARE(record["name"].toString(), QString("Alice"));
        QCOMPARE(record["age"].toInt(), 25);
        QCOMPARE(record["salary"].toDouble(), 50000.50);
    } else {
        QFAIL("Query should return at least one row");
    }
}

void TestResultHandler::testConvertFromSqlType()
{
    // 测试基本类型转换（无目标类型）
    QCOMPARE(m_handler->convertFromSqlType(QVariant(42)).toInt(), 42);
    QCOMPARE(m_handler->convertFromSqlType(QVariant(3.14)).toDouble(), 3.14);
    QCOMPARE(m_handler->convertFromSqlType(QVariant("Hello")).toString(), QString("Hello"));
    QCOMPARE(m_handler->convertFromSqlType(QVariant(true)).toBool(), true);
    
    // 测试NULL值
    QVERIFY(m_handler->convertFromSqlType(QVariant()).isNull());
}

void TestResultHandler::testConvertFromSqlTypeWithTargetTypes()
{
    // 测试指定目标类型的转换
    QCOMPARE(m_handler->convertFromSqlType(QVariant("42"), "int").toInt(), 42);
    QCOMPARE(m_handler->convertFromSqlType(QVariant(42), "string").toString(), QString("42"));
    QCOMPARE(m_handler->convertFromSqlType(QVariant("3.14"), "double").toDouble(), 3.14);
    QCOMPARE(m_handler->convertFromSqlType(QVariant(1), "bool").toBool(), true);
    QCOMPARE(m_handler->convertFromSqlType(QVariant(0), "bool").toBool(), false);
    
    // 测试日期时间转换
    QDateTime now = QDateTime::currentDateTime();
    QCOMPARE(m_handler->convertFromSqlType(QVariant(now), "datetime").toDateTime(), now);
    
    QDate today = QDate::currentDate();
    QCOMPARE(m_handler->convertFromSqlType(QVariant(today), "date").toDate(), today);
    
    // 测试UUID转换
    QString uuidStr = "550e8400-e29b-41d4-a716-446655440000";
    QUuid expectedUuid = QUuid::fromString(uuidStr);
    QVariant convertedUuid = m_handler->convertFromSqlType(QVariant(uuidStr), "uuid");
    QCOMPARE(convertedUuid.toUuid(), expectedUuid);
}

void TestResultHandler::testConvertFromSqlTypeJson()
{
    // 测试JSON对象转换
    QString jsonObjectStr = R"({"name": "Alice", "age": 25, "active": true})";
    QVariant result = m_handler->convertFromSqlType(QVariant(jsonObjectStr), "json");
    
    QVERIFY(result.canConvert<QVariantMap>());
    QVariantMap jsonMap = result.toMap();
    QCOMPARE(jsonMap["name"].toString(), QString("Alice"));
    QCOMPARE(jsonMap["age"].toInt(), 25);
    QCOMPARE(jsonMap["active"].toBool(), true);
    
    // 测试无效JSON
    QString invalidJson = "invalid json";
    QVariant invalidResult = m_handler->convertFromSqlType(QVariant(invalidJson), "json");
    QCOMPARE(invalidResult.toString(), invalidJson); // 应该返回原始字符串
}

void TestResultHandler::testConvertFromSqlTypeArray()
{
    // 测试JSON数组转换
    QString jsonArrayStr = R"(["item1", "item2", "item3"])";
    QVariant result = m_handler->convertFromSqlType(QVariant(jsonArrayStr), "array");
    
    QVERIFY(result.canConvert<QVariantList>());
    QVariantList jsonList = result.toList();
    QCOMPARE(jsonList.size(), 3);
    QCOMPARE(jsonList[0].toString(), QString("item1"));
    QCOMPARE(jsonList[1].toString(), QString("item2"));
    QCOMPARE(jsonList[2].toString(), QString("item3"));
}

void TestResultHandler::testGetColumnNames()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT name, age, salary FROM test_data LIMIT 1");
    
    QStringList columnNames = m_handler->getColumnNames(query);
    QCOMPARE(columnNames.size(), 3);
    QVERIFY(columnNames.contains("name"));
    QVERIFY(columnNames.contains("age"));
    QVERIFY(columnNames.contains("salary"));
}

void TestResultHandler::testErrorHandling()
{
    // 测试查询错误处理
    QSqlQuery invalidQuery(*m_connection);
    invalidQuery.exec("SELECT * FROM non_existent_table");
    
    // handleSingleResult应该抛出异常
    QVERIFY_EXCEPTION_THROWN(m_handler->handleSingleResult(invalidQuery), SqlExecutionException);
    
    // handleListResult应该抛出异常
    QVERIFY_EXCEPTION_THROWN(m_handler->handleListResult(invalidQuery), SqlExecutionException);
}

void TestResultHandler::testNullValues()
{
    QSqlQuery query(*m_connection);
    query.exec("SELECT * FROM test_data WHERE name = 'Charlie'");
    
    QVariant result = m_handler->handleSingleResult(query);
    QVERIFY(!result.isNull());
    
    QVariantMap record = result.toMap();
    QCOMPARE(record["name"].toString(), QString("Charlie"));
    QVERIFY(record["age"].isNull());
    QVERIFY(record["salary"].isNull());
    QVERIFY(record["active"].isNull());
    QVERIFY(record["birth_date"].isNull());
    QVERIFY(record["created_at"].isNull());
    QVERIFY(record["json_data"].isNull());
}

void TestResultHandler::testComplexDataTypes()
{
    // 测试二进制数据
    QByteArray binaryData("Hello World");
    QVariant convertedBinary = m_handler->convertFromSqlType(QVariant(binaryData), "binary");
    QCOMPARE(convertedBinary.toByteArray(), binaryData);
    
    // 测试长整型
    qint64 longValue = 9223372036854775807LL; // 最大long long值
    QVariant convertedLong = m_handler->convertFromSqlType(QVariant(longValue), "long");
    QCOMPARE(convertedLong.toLongLong(), longValue);
    
    // 测试时间类型
    QTime currentTime = QTime::currentTime();
    QVariant convertedTime = m_handler->convertFromSqlType(QVariant(currentTime), "time");
    QCOMPARE(convertedTime.toTime(), currentTime);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestResultHandler test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_resulthandler_test.moc"