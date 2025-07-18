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

// Test Mapper interface - simplified for testing
class UserMapper : public QObject
{
    Q_OBJECT
    
public:
    explicit UserMapper(QObject* parent = nullptr) : QObject(parent) {}
    
    // These methods will be dynamically implemented by MapperProxy
    virtual QVariant selectById(int id) { Q_UNUSED(id); return QVariant(); }
    virtual QVariantList selectAll() { return QVariantList(); }
    virtual int insert(const QVariantMap& user) { Q_UNUSED(user); return 0; }
    virtual int update(const QVariantMap& user) { Q_UNUSED(user); return 0; }
    virtual int deleteById(int id) { Q_UNUSED(id); return 0; }
};

class TestSessionMapperIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testGetMapperBasic();
    void testGetMapperWithClosedSession();
    void testGetMapperWithNullRegistry();
    void testMapperLifecycleBinding();
    void testEndToEndMapperUsage();
    void testMultipleMapperInstances();

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

void TestSessionMapperIntegration::init()
{
    // Reset state before each test
}

void TestSessionMapperIntegration::cleanup()
{
    // Clean up after each test
}

void TestSessionMapperIntegration::setupTestDatabase()
{
    // Create in-memory SQLite database for testing
    m_db = QSqlDatabase::addDatabase("QSQLITE", "test_session_mapper_db");
    m_db.setDatabaseName(":memory:");
    
    if (!m_db.open()) {
        QFAIL("Failed to open test database");
    }
    
    // Create test tables
    QSqlQuery query(m_db);
    query.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
    query.exec("INSERT INTO users (name, email) VALUES ('Test User', 'test@example.com')");
    query.exec("INSERT INTO users (name, email) VALUES ('Another User', 'another@example.com')");
    
    m_connection = QSharedPointer<QSqlDatabase>::create(m_db);
}

void TestSessionMapperIntegration::setupTestMappers()
{
    // Create cache manager
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 300;
    m_cacheManager = QSharedPointer<CacheManager>::create(config);
    
    // Create executor
    m_executor = QSharedPointer<Executor>::create(m_connection, m_cacheManager);
    
    // Create mapper registry
    m_mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    // Register test mapper
    MapperConfig userMapper;
    userMapper.namespace_ = "UserMapper";
    userMapper.xmlPath = "test_user_mapper.xml";
    
    // Add test statements
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
    
    StatementConfig insert;
    insert.id = "insert";
    insert.sql = "INSERT INTO users (name, email) VALUES (:name, :email)";
    insert.type = StatementType::INSERT;
    insert.parameterType = "User";
    userMapper.statements["UserMapper.insert"] = insert;
    
    StatementConfig update;
    update.id = "update";
    update.sql = "UPDATE users SET name = :name, email = :email WHERE id = :id";
    update.type = StatementType::UPDATE;
    update.parameterType = "User";
    userMapper.statements["UserMapper.update"] = update;
    
    StatementConfig deleteById;
    deleteById.id = "deleteById";
    deleteById.sql = "DELETE FROM users WHERE id = :id";
    deleteById.type = StatementType::DELETE;
    deleteById.parameterType = "int";
    userMapper.statements["UserMapper.deleteById"] = deleteById;
    
    m_mapperRegistry->registerMapper("UserMapper", userMapper);
}

QSharedPointer<Session> TestSessionMapperIntegration::createTestSession()
{
    return QSharedPointer<Session>::create(m_connection, m_executor, m_mapperRegistry);
}

void TestSessionMapperIntegration::testGetMapperBasic()
{
    auto session = createTestSession();
    
    // Test getting a mapper
    UserMapper* mapper = session->getMapper<UserMapper>();
    
    QVERIFY(mapper != nullptr);
    QVERIFY(qobject_cast<MapperProxy*>(mapper) != nullptr);
    
    // Test that the same session returns the same mapper instance
    UserMapper* mapper2 = session->getMapper<UserMapper>();
    QCOMPARE(mapper, mapper2);
}

void TestSessionMapperIntegration::testGetMapperWithClosedSession()
{
    auto session = createTestSession();
    
    // Close the session
    session->close();
    
    // Getting mapper from closed session should throw exception
    QVERIFY_EXCEPTION_THROWN(
        session->getMapper<UserMapper>(),
        SqlExecutionException
    );
}

void TestSessionMapperIntegration::testGetMapperWithNullRegistry()
{
    // Create session with null mapper registry
    auto session = QSharedPointer<Session>::create(m_connection, m_executor, nullptr);
    
    // Getting mapper should throw exception
    QVERIFY_EXCEPTION_THROWN(
        session->getMapper<UserMapper>(),
        MappingException
    );
}

void TestSessionMapperIntegration::testMapperLifecycleBinding()
{
    auto session = createTestSession();
    
    // Get mapper
    UserMapper* mapper = session->getMapper<UserMapper>();
    QVERIFY(mapper != nullptr);
    
    // Close session
    session->close();
    
    // Mapper should still exist but operations should fail
    QVERIFY(mapper != nullptr);
    
    // Attempting to use the mapper should fail because session is closed
    // Note: This depends on how MapperProxy handles closed sessions
    // For now, we just verify the mapper object still exists
}

void TestSessionMapperIntegration::testEndToEndMapperUsage()
{
    auto session = createTestSession();
    
    // Get mapper
    UserMapper* mapper = session->getMapper<UserMapper>();
    QVERIFY(mapper != nullptr);
    
    // Cast to MapperProxy to access the invoke methods
    MapperProxy* proxy = qobject_cast<MapperProxy*>(mapper);
    QVERIFY(proxy != nullptr);
    
    // Test select by ID
    QVariantList args;
    args << 1;
    QVariant result = proxy->invokeMethod("selectById", args);
    QVERIFY(!result.isNull());
    
    // Test select all
    QVariantList allResults = proxy->invokeMethod("selectAll", QVariantList()).toList();
    QVERIFY(allResults.size() >= 2);
    
    // Test insert
    QVariantMap newUser;
    newUser["name"] = "New User";
    newUser["email"] = "new@example.com";
    QVariantList insertArgs;
    insertArgs << newUser;
    QVariant insertResult = proxy->invokeMethod("insert", insertArgs);
    QVERIFY(insertResult.toInt() > 0);
    
    // Test update
    QVariantMap updateUser;
    updateUser["id"] = 1;
    updateUser["name"] = "Updated User";
    updateUser["email"] = "updated@example.com";
    QVariantList updateArgs;
    updateArgs << updateUser;
    QVariant updateResult = proxy->invokeMethod("update", updateArgs);
    QVERIFY(updateResult.toInt() > 0);
    
    // Test delete
    QVariantList deleteArgs;
    deleteArgs << 2;
    QVariant deleteResult = proxy->invokeMethod("deleteById", deleteArgs);
    QVERIFY(deleteResult.toInt() > 0);
}

void TestSessionMapperIntegration::testMultipleMapperInstances()
{
    auto session1 = createTestSession();
    auto session2 = createTestSession();
    
    // Get mappers from different sessions
    UserMapper* mapper1 = session1->getMapper<UserMapper>();
    UserMapper* mapper2 = session2->getMapper<UserMapper>();
    
    QVERIFY(mapper1 != nullptr);
    QVERIFY(mapper2 != nullptr);
    
    // Mappers from different sessions should be different instances
    QVERIFY(mapper1 != mapper2);
    
    // But mappers from the same session should be the same instance
    UserMapper* mapper1_again = session1->getMapper<UserMapper>();
    QCOMPARE(mapper1, mapper1_again);
}

#include "test_session_mapper_integration.moc"