#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestSession : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testSessionCreation();
    void testBasicCRUDOperations();
    void testTransactionManagement();
    void testSessionClosure();
    void testErrorHandling();
    void testStatementIdParsing();

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

void TestSession::initTestCase()
{
    setupTestDatabase();
    setupTestMappers();
}

void TestSession::cleanupTestCase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("test_session_db");
}

void TestSession::init()
{
    // 每个测试前重置状态
}

void TestSession::cleanup()
{
    // 每个测试后清理
}

void TestSession::setupTestDatabase()
{
    // 创建内存SQLite数据库用于测试
    m_db = QSqlDatabase::addDatabase("QSQLITE", "test_session_db");
    m_db.setDatabaseName(":memory:");
    
    if (!m_db.open()) {
        QFAIL("Failed to open test database");
    }
    
    // 创建测试表
    QSqlQuery query(m_db);
    query.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
    query.exec("INSERT INTO users (name, email) VALUES ('Test User', 'test@example.com')");
    query.exec("INSERT INTO users (name, email) VALUES ('Another User', 'another@example.com')");
    
    m_connection = QSharedPointer<QSqlDatabase>::create(m_db);
}

void TestSession::setupTestMappers()
{
    // 创建缓存管理器
    DatabaseConfig config;
    config.cacheEnabled = true;
    config.maxCacheSize = 100;
    config.cacheExpireTime = 300;
    m_cacheManager = QSharedPointer<CacheManager>::create(config);
    
    // 创建执行器
    m_executor = QSharedPointer<Executor>::create(m_connection, m_cacheManager);
    
    // 创建Mapper注册表
    m_mapperRegistry = QSharedPointer<MapperRegistry>::create();
    
    // 注册测试Mapper
    MapperConfig userMapper;
    userMapper.namespace_ = "UserMapper";
    userMapper.xmlPath = "test_user_mapper.xml";
    
    // 添加测试语句
    StatementConfig selectUser;
    selectUser.id = "selectById";
    selectUser.sql = "SELECT * FROM users WHERE id = :id";
    selectUser.type = StatementType::SELECT;
    selectUser.parameterType = "int";
    selectUser.resultType = "User";
    userMapper.statements["selectById"] = selectUser;
    
    StatementConfig selectAllUsers;
    selectAllUsers.id = "selectAll";
    selectAllUsers.sql = "SELECT * FROM users";
    selectAllUsers.type = StatementType::SELECT;
    selectAllUsers.resultType = "List<User>";
    userMapper.statements["selectAll"] = selectAllUsers;
    
    StatementConfig insertUser;
    insertUser.id = "insert";
    insertUser.sql = "INSERT INTO users (name, email) VALUES (:name, :email)";
    insertUser.type = StatementType::INSERT;
    insertUser.parameterType = "User";
    userMapper.statements["insert"] = insertUser;
    
    StatementConfig updateUser;
    updateUser.id = "update";
    updateUser.sql = "UPDATE users SET name = :name, email = :email WHERE id = :id";
    updateUser.type = StatementType::UPDATE;
    updateUser.parameterType = "User";
    userMapper.statements["update"] = updateUser;
    
    StatementConfig deleteUser;
    deleteUser.id = "delete";
    deleteUser.sql = "DELETE FROM users WHERE id = :id";
    deleteUser.type = StatementType::DELETE;
    deleteUser.parameterType = "int";
    userMapper.statements["delete"] = deleteUser;
    
    m_mapperRegistry->registerMapper("UserMapper", userMapper);
}

QSharedPointer<Session> TestSession::createTestSession()
{
    return QSharedPointer<Session>::create(m_connection, m_executor, m_mapperRegistry);
}

void TestSession::testSessionCreation()
{
    auto session = createTestSession();
    
    QVERIFY(session != nullptr);
    QVERIFY(!session->isClosed());
    QVERIFY(!session->isInTransaction());
}

void TestSession::testBasicCRUDOperations()
{
    auto session = createTestSession();
    
    // 测试查询单个记录
    QVariantMap params;
    params["id"] = 1;
    QVariant result = session->selectOne("UserMapper.selectById", params);
    QVERIFY(!result.isNull());
    
    // 测试查询列表
    QVariantList results = session->selectList("UserMapper.selectAll");
    QVERIFY(results.size() >= 2);
    
    // 测试插入
    QVariantMap insertParams;
    insertParams["name"] = "New User";
    insertParams["email"] = "new@example.com";
    int insertResult = session->insert("UserMapper.insert", insertParams);
    QVERIFY(insertResult > 0);
    
    // 测试更新
    QVariantMap updateParams;
    updateParams["id"] = 1;
    updateParams["name"] = "Updated User";
    updateParams["email"] = "updated@example.com";
    int updateResult = session->update("UserMapper.update", updateParams);
    QVERIFY(updateResult > 0);
    
    // 测试删除
    QVariantMap deleteParams;
    deleteParams["id"] = 2;
    int deleteResult = session->remove("UserMapper.delete", deleteParams);
    QVERIFY(deleteResult > 0);
}

void TestSession::testTransactionManagement()
{
    auto session = createTestSession();
    
    // 测试开始事务
    QVERIFY(!session->isInTransaction());
    session->beginTransaction();
    QVERIFY(session->isInTransaction());
    
    // 在事务中执行操作
    QVariantMap params;
    params["name"] = "Transaction User";
    params["email"] = "transaction@example.com";
    session->insert("UserMapper.insert", params);
    
    // 测试回滚
    session->rollback();
    QVERIFY(!session->isInTransaction());
    
    // 测试提交
    session->beginTransaction();
    session->insert("UserMapper.insert", params);
    session->commit();
    QVERIFY(!session->isInTransaction());
}

void TestSession::testSessionClosure()
{
    auto session = createTestSession();
    
    QVERIFY(!session->isClosed());
    
    // 在事务中关闭会话应该自动回滚
    session->beginTransaction();
    session->close();
    
    QVERIFY(session->isClosed());
    QVERIFY(!session->isInTransaction());
    
    // 关闭后的操作应该抛出异常
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("UserMapper.selectById", QVariantMap()),
        SqlExecutionException
    );
}

void TestSession::testErrorHandling()
{
    auto session = createTestSession();
    
    // 测试无效的语句ID格式
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("InvalidFormat", QVariantMap()),
        MappingException
    );
    
    // 测试不存在的Mapper
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("NonExistentMapper.selectById", QVariantMap()),
        MappingException
    );
    
    // 测试不存在的语句
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("UserMapper.nonExistentStatement", QVariantMap()),
        MappingException
    );
}

void TestSession::testStatementIdParsing()
{
    auto session = createTestSession();
    
    // 测试正确的语句ID格式
    QVariantMap params;
    params["id"] = 1;
    
    // 这应该成功执行
    try {
        QVariant result = session->selectOne("UserMapper.selectById", params);
        QVERIFY(!result.isNull());
    } catch (...) {
        QFAIL("Should not throw exception for valid statement ID");
    }
    
    // 测试错误的格式
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("UserMapper", params),
        MappingException
    );
    
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("UserMapper.statement.extra", params),
        MappingException
    );
}

QTEST_MAIN(TestSession)
#include "run_session_test.moc"