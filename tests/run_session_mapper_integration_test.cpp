#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/session_impl.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/mapperproxy.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

// Simple test Mapper interface
class UserMapper : public QObject
{
    Q_OBJECT
    
public:
    explicit UserMapper(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~UserMapper() = default;
};

class TestSessionMapperIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testGetMapperBasic();
    void testGetMapperWithClosedSession();
    void testGetMapperWithNullRegistry();
    void testEndToEndMapperUsage();

private:
    void setupTestDatabase();
    void setupTestMappers();
    QSharedPointer<Session> createTestSession();
    
    QSqlDatabase m_db;
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<Executor> m_executor;
    QSharedPointer<MapperRegistry> m_mapperRegistry;
    QSharedPointer<CacheManager> m_cacheManager;
};

void TestSessionMapperIntegration::initTestCase()
{
    setupTestDatabase();
    setupTestMappers();
}

void TestSessionMapperIntegration::cleanupTestCase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("test_session_mapper_db");
}

void TestSessionMapperIntegration::setupTestDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "test_session_mapper_db");
    m_db.setDatabaseName(":memory:");
    
    if (!m_db.open()) {
        QFAIL("Failed to open test database");
    }
    
    QSqlQuery query(m_db);
    query.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
    query.exec("INSERT INTO users (name, email) VALUES ('Test User', 'test@example.com')");
    query.exec("INSERT INTO users (name, email) VALUES ('Another User', 'another@example.com')");
    
    m_connection = QSharedPointer<QSqlDatabase>::create(m_db);
}

void TestSessionMapperIntegration::setupTestMappers()
{
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 300;
    m_cacheManager = QSharedPointer<CacheManager>::create(config);
    
    m_executor = QSharedPointer<Executor>::create(m_connection, m_cacheManager);
    m_mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    MapperConfig userMapper;
    userMapper.namespace_ = "UserMapper";
    userMapper.xmlPath = "test_user_mapper.xml";
    
    StatementConfig selectById;
    selectById.id = "selectById";
    selectById.sql = "SELECT * FROM users WHERE id = :id";
    selectById.type = StatementType::SELECT;
    selectById.parameterType = "int";
    selectById.resultType = "User";
    userMapper.statements["UserMapper.selectById"] = selectById;
    
    StatementConfig selectAll;
    selectAll.id = "selectAll";
    selectAll.sql = "SELECT * FROM users";
    selectAll.type = StatementType::SELECT;
    selectAll.resultType = "List<User>";
    userMapper.statements["UserMapper.selectAll"] = selectAll;
    
    m_mapperRegistry->registerMapper("UserMapper", userMapper);
}

QSharedPointer<Session> TestSessionMapperIntegration::createTestSession()
{
    return QSharedPointer<Session>::create(m_connection, m_executor, m_mapperRegistry);
}

void TestSessionMapperIntegration::testGetMapperBasic()
{
    auto session = createTestSession();
    
    // In the current design, we get MapperProxy directly
    // The template parameter is used for type inference but returns MapperProxy
    QObject* mapperObj = session->getMapper<UserMapper>();
    
    // The returned object should be a MapperProxy
    MapperProxy* mapper = qobject_cast<MapperProxy*>(mapperObj);
    QVERIFY(mapper != nullptr);
    
    // Verify it's the same instance on subsequent calls
    QObject* mapperObj2 = session->getMapper<UserMapper>();
    QCOMPARE(mapperObj, mapperObj2);
    
    // Verify the mapper has the correct name
    QCOMPARE(mapper->getMapperName(), QString("UserMapper"));
}

void TestSessionMapperIntegration::testGetMapperWithClosedSession()
{
    auto session = createTestSession();
    session->close();
    
    QVERIFY_EXCEPTION_THROWN(
        session->getMapper<UserMapper>(),
        SqlExecutionException
    );
}

void TestSessionMapperIntegration::testGetMapperWithNullRegistry()
{
    auto session = QSharedPointer<Session>::create(m_connection, m_executor, nullptr);
    
    QVERIFY_EXCEPTION_THROWN(
        session->getMapper<UserMapper>(),
        MappingException
    );
}

void TestSessionMapperIntegration::testEndToEndMapperUsage()
{
    auto session = createTestSession();
    
    QObject* mapperObj = session->getMapper<UserMapper>();
    QVERIFY(mapperObj != nullptr);
    
    MapperProxy* proxy = qobject_cast<MapperProxy*>(mapperObj);
    QVERIFY(proxy != nullptr);
    
    // Test with proper parameter mapping
    QVariantMap params;
    params["id"] = 1;
    QVariantList args;
    args << params;
    QVariant result = proxy->invokeMethod("selectById", args);
    QVERIFY(!result.isNull());
    
    QVariantList allResults = proxy->invokeMethod("selectAll", QVariantList()).toList();
    QVERIFY(allResults.size() >= 2);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestSessionMapperIntegration test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_session_mapper_integration_test.moc"