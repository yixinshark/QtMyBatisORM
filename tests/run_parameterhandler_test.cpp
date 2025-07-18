#include <QtTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "QtMyBatisORM/parameterhandler.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestParameterHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testConstructor();
    void testSetParametersWithNamedParams();
    void testSetParametersWithPositionalParams();
    void testSetParametersWithMixedParams();
    void testSetParametersWithEmptySQL();
    void testSetParametersWithNoPlaceholders();
    void testConvertParameter();
    void testConvertToSqlType();
    void testBindByNameSuccess();
    void testBindByNameMissingParams();
    void testBindByNameExtraParams();
    void testBindByIndexSuccess();
    void testBindByIndexMismatch();
    void testBindByIndexNumericKeys();
    void testComplexDataTypes();
    void testJsonConversion();
    void testParameterValidation();
    void testErrorHandling();

private:
    void setupTestDatabase();
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<ParameterHandler> m_handler;
    QString m_connectionName;
};

void TestParameterHandler::initTestCase()
{
    m_connectionName = "test_parameterhandler_connection";
    setupTestDatabase();
}

void TestParameterHandler::cleanupTestCase()
{
    if (m_connection && m_connection->isOpen()) {
        m_connection->close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void TestParameterHandler::init()
{
    m_handler = QSharedPointer<ParameterHandler>::create();
}

void TestParameterHandler::cleanup()
{
    m_handler.reset();
}

void TestParameterHandler::setupTestDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        QFAIL("Failed to open test database");
    }
    
    m_connection = QSharedPointer<QSqlDatabase>::create(db);
}

void TestParameterHandler::testConstructor()
{
    QVERIFY(m_handler != nullptr);
}

void TestParameterHandler::testSetParametersWithNamedParams()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = :name AND age = :age");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    parameters["age"] = 25;
    
    // 这应该成功执行而不抛出异常
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testSetParametersWithPositionalParams()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = ? AND age = ?");
    
    QVariantMap parameters;
    parameters["0"] = "Alice";
    parameters["1"] = 25;
    
    // 这应该成功执行而不抛出异常
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testSetParametersWithMixedParams()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = :name AND age = ?");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    parameters["0"] = 25;
    
    // 混合参数应该使用命名参数处理
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testSetParametersWithEmptySQL()
{
    QSqlQuery query(*m_connection);
    // 不准备任何SQL
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    
    QVERIFY_EXCEPTION_THROWN(m_handler->setParameters(query, parameters), MappingException);
}

void TestParameterHandler::testSetParametersWithNoPlaceholders()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT COUNT(*) FROM users");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";  // 提供了参数但SQL中没有占位符
    
    // 应该记录警告但不抛出异常
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testConvertParameter()
{
    // 测试不同类型的转换
    QCOMPARE(m_handler->convertParameter(QVariant(42), "int").toInt(), 42);
    QCOMPARE(m_handler->convertParameter(QVariant("42"), "int").toInt(), 42);
    QCOMPARE(m_handler->convertParameter(QVariant(3.14), "double").toDouble(), 3.14);
    QCOMPARE(m_handler->convertParameter(QVariant("Hello"), "string").toString(), QString("Hello"));
    QCOMPARE(m_handler->convertParameter(QVariant(true), "bool").toBool(), true);
    
    // 测试日期时间转换
    QDateTime now = QDateTime::currentDateTime();
    QCOMPARE(m_handler->convertParameter(QVariant(now), "datetime").toDateTime(), now);
}

void TestParameterHandler::testConvertToSqlType()
{
    // 测试基本类型转换
    QCOMPARE(m_handler->convertParameter(QVariant(42)).toInt(), 42);
    QCOMPARE(m_handler->convertParameter(QVariant(3.14)).toDouble(), 3.14);
    QCOMPARE(m_handler->convertParameter(QVariant("Hello")).toString(), QString("Hello"));
    QCOMPARE(m_handler->convertParameter(QVariant(true)).toBool(), true);
    
    // 测试无效值
    QVERIFY(m_handler->convertParameter(QVariant()).isNull());
    
    // 测试UUID
    QUuid uuid = QUuid::createUuid();
    QCOMPARE(m_handler->convertParameter(QVariant(uuid)).toString(), uuid.toString());
}

void TestParameterHandler::testBindByNameSuccess()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = :name AND age = :age");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    parameters["age"] = 25;
    
    // 直接测试bindByName方法（通过setParameters间接测试）
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testBindByNameMissingParams()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = :name AND age = :age");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    // 缺少age参数
    
    QVERIFY_EXCEPTION_THROWN(m_handler->setParameters(query, parameters), MappingException);
}

void TestParameterHandler::testBindByNameExtraParams()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = :name");
    
    QVariantMap parameters;
    parameters["name"] = "Alice";
    parameters["age"] = 25;  // 额外的参数
    
    // 应该记录警告但不抛出异常
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testBindByIndexSuccess()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = ? AND age = ?");
    
    QVariantMap parameters;
    parameters["0"] = "Alice";
    parameters["1"] = 25;
    
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testBindByIndexMismatch()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = ? AND age = ?");
    
    QVariantMap parameters;
    parameters["0"] = "Alice";
    // 缺少第二个参数
    
    QVERIFY_EXCEPTION_THROWN(m_handler->setParameters(query, parameters), MappingException);
}

void TestParameterHandler::testBindByIndexNumericKeys()
{
    QSqlQuery query(*m_connection);
    query.prepare("SELECT * FROM users WHERE name = ? AND age = ? AND email = ?");
    
    QVariantMap parameters;
    parameters["2"] = "alice@example.com";  // 数字键，但不按顺序
    parameters["0"] = "Alice";
    parameters["1"] = 25;
    
    try {
        m_handler->setParameters(query, parameters);
        QVERIFY(true); // 如果没有异常，测试通过
    } catch (const QtMyBatisException&) {
        QFAIL("Unexpected exception thrown");
    }
}

void TestParameterHandler::testComplexDataTypes()
{
    // 测试复杂数据类型的转换
    QVariantList list;
    list << "item1" << "item2" << "item3";
    
    QVariant convertedList = m_handler->convertParameter(list);
    QVERIFY(convertedList.canConvert<QString>());
    
    // 验证JSON格式
    QJsonDocument doc = QJsonDocument::fromJson(convertedList.toByteArray());
    QVERIFY(doc.isArray());
    QJsonArray jsonArray = doc.array();
    QCOMPARE(jsonArray.size(), 3);
    QCOMPARE(jsonArray[0].toString(), QString("item1"));
}

void TestParameterHandler::testJsonConversion()
{
    // 测试Map到JSON的转换
    QVariantMap map;
    map["name"] = "Alice";
    map["age"] = 25;
    map["active"] = true;
    
    QVariant convertedMap = m_handler->convertParameter(map);
    QVERIFY(convertedMap.canConvert<QString>());
    
    // 验证JSON格式
    QJsonDocument doc = QJsonDocument::fromJson(convertedMap.toByteArray());
    QVERIFY(doc.isObject());
    QJsonObject jsonObject = doc.object();
    QCOMPARE(jsonObject["name"].toString(), QString("Alice"));
    QCOMPARE(jsonObject["age"].toInt(), 25);
    QCOMPARE(jsonObject["active"].toBool(), true);
}

void TestParameterHandler::testParameterValidation()
{
    // 测试参数名验证
    QVERIFY(m_handler->isValidParameterName(":name"));
    QVERIFY(m_handler->isValidParameterName(":user_id"));
    QVERIFY(m_handler->isValidParameterName(":param123"));
    
    QVERIFY(!m_handler->isValidParameterName("name"));  // 缺少冒号
    QVERIFY(!m_handler->isValidParameterName(":"));     // 只有冒号
    QVERIFY(!m_handler->isValidParameterName(":123"));  // 以数字开头
    QVERIFY(!m_handler->isValidParameterName(":na-me")); // 包含连字符
}

void TestParameterHandler::testErrorHandling()
{
    // 测试各种错误情况
    QSqlQuery query(*m_connection);
    
    // 测试空SQL的错误处理
    QVariantMap parameters;
    parameters["name"] = "Alice";
    
    QVERIFY_EXCEPTION_THROWN(m_handler->setParameters(query, parameters), MappingException);
    
    // 测试位置参数数量不匹配
    query.prepare("SELECT * FROM users WHERE name = ?");
    parameters.clear();
    parameters["0"] = "Alice";
    parameters["1"] = "Extra";  // 额外的参数
    
    QVERIFY_EXCEPTION_THROWN(m_handler->setParameters(query, parameters), MappingException);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestParameterHandler test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_parameterhandler_test.moc"