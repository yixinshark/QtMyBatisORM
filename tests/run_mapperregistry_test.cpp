#include <QCoreApplication>
#include <QtTest>
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestMapperRegistry : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // 基本功能测试
    void testRegisterMapper();
    void testRegisterMappers();
    void testGetMapperConfig();
    void testHasMapper();
    void testGetMapperNames();
    void testGetMapperCount();
    void testClear();
    
    // 验证功能测试
    void testValidateMapper();
    void testValidateAllMappers();
    void testGetStatementIds();
    
    // 异常处理测试
    void testDuplicateMapperRegistration();
    void testGetNonExistentMapper();
    void testValidateInvalidMapper();

private:
    MapperRegistry* registry;
    MapperConfig createTestMapperConfig(const QString& namespace_);
    StatementConfig createTestStatementConfig(const QString& id, StatementType type);
};

void TestMapperRegistry::initTestCase()
{
    // 测试套件初始化
}

void TestMapperRegistry::cleanupTestCase()
{
    // 测试套件清理
}

void TestMapperRegistry::init()
{
    registry = new MapperRegistry();
}

void TestMapperRegistry::cleanup()
{
    delete registry;
    registry = nullptr;
}

void TestMapperRegistry::testRegisterMapper()
{
    MapperConfig config = createTestMapperConfig("TestMapper");
    
    registry->registerMapper("TestMapper", config);
    
    QVERIFY(registry->hasMapper("TestMapper"));
    QCOMPARE(registry->getMapperNames().size(), 1);
    QCOMPARE(registry->getMapperCount(), 1);
}

void TestMapperRegistry::testRegisterMappers()
{
    QList<MapperConfig> configs;
    configs.append(createTestMapperConfig("UserMapper"));
    configs.append(createTestMapperConfig("OrderMapper"));
    configs.append(createTestMapperConfig("ProductMapper"));
    
    registry->registerMappers(configs);
    
    QCOMPARE(registry->getMapperCount(), 3);
    QVERIFY(registry->hasMapper("UserMapper"));
    QVERIFY(registry->hasMapper("OrderMapper"));
    QVERIFY(registry->hasMapper("ProductMapper"));
}

void TestMapperRegistry::testGetMapperConfig()
{
    MapperConfig config = createTestMapperConfig("TestMapper");
    registry->registerMapper("TestMapper", config);
    
    MapperConfig retrieved = registry->getMapperConfig("TestMapper");
    QCOMPARE(retrieved.namespace_, QString("TestMapper"));
    QCOMPARE(retrieved.xmlPath, QString(":/mappers/TestMapper.xml"));
    QVERIFY(!retrieved.statements.isEmpty());
}

void TestMapperRegistry::testHasMapper()
{
    QVERIFY(!registry->hasMapper("NonExistentMapper"));
    
    MapperConfig config = createTestMapperConfig("TestMapper");
    registry->registerMapper("TestMapper", config);
    
    QVERIFY(registry->hasMapper("TestMapper"));
    QVERIFY(!registry->hasMapper("AnotherMapper"));
}

void TestMapperRegistry::testGetMapperNames()
{
    QVERIFY(registry->getMapperNames().isEmpty());
    
    registry->registerMapper("UserMapper", createTestMapperConfig("UserMapper"));
    registry->registerMapper("OrderMapper", createTestMapperConfig("OrderMapper"));
    
    QStringList names = registry->getMapperNames();
    QCOMPARE(names.size(), 2);
    QVERIFY(names.contains("UserMapper"));
    QVERIFY(names.contains("OrderMapper"));
}

void TestMapperRegistry::testGetMapperCount()
{
    QCOMPARE(registry->getMapperCount(), 0);
    
    registry->registerMapper("TestMapper1", createTestMapperConfig("TestMapper1"));
    QCOMPARE(registry->getMapperCount(), 1);
    
    registry->registerMapper("TestMapper2", createTestMapperConfig("TestMapper2"));
    QCOMPARE(registry->getMapperCount(), 2);
}

void TestMapperRegistry::testClear()
{
    registry->registerMapper("TestMapper1", createTestMapperConfig("TestMapper1"));
    registry->registerMapper("TestMapper2", createTestMapperConfig("TestMapper2"));
    
    QCOMPARE(registry->getMapperCount(), 2);
    
    registry->clear();
    
    QCOMPARE(registry->getMapperCount(), 0);
    QVERIFY(registry->getMapperNames().isEmpty());
    QVERIFY(!registry->hasMapper("TestMapper1"));
    QVERIFY(!registry->hasMapper("TestMapper2"));
}

void TestMapperRegistry::testValidateMapper()
{
    // 测试有效的Mapper
    MapperConfig validConfig = createTestMapperConfig("ValidMapper");
    registry->registerMapper("ValidMapper", validConfig);
    
    QVERIFY(registry->validateMapper("ValidMapper"));
    
    // 测试无效的Mapper（空命名空间）
    MapperConfig invalidConfig;
    invalidConfig.namespace_ = "";  // 空命名空间
    invalidConfig.xmlPath = ":/mappers/Invalid.xml";
    registry->registerMapper("InvalidMapper", invalidConfig);
    
    QVERIFY(!registry->validateMapper("InvalidMapper"));
    
    // 测试不存在的Mapper
    QVERIFY(!registry->validateMapper("NonExistentMapper"));
}

void TestMapperRegistry::testValidateAllMappers()
{
    // 注册有效的Mapper
    registry->registerMapper("ValidMapper1", createTestMapperConfig("ValidMapper1"));
    registry->registerMapper("ValidMapper2", createTestMapperConfig("ValidMapper2"));
    
    QVERIFY(registry->validateAllMappers());
    
    // 添加无效的Mapper
    MapperConfig invalidConfig;
    invalidConfig.namespace_ = "";  // 空命名空间
    registry->registerMapper("InvalidMapper", invalidConfig);
    
    QVERIFY(!registry->validateAllMappers());
}

void TestMapperRegistry::testGetStatementIds()
{
    MapperConfig config = createTestMapperConfig("TestMapper");
    registry->registerMapper("TestMapper", config);
    
    QStringList statementIds = registry->getStatementIds("TestMapper");
    QVERIFY(!statementIds.isEmpty());
    QVERIFY(statementIds.contains("selectUser"));
    QVERIFY(statementIds.contains("insertUser"));
    
    // 测试不存在的Mapper
    QStringList emptyIds = registry->getStatementIds("NonExistentMapper");
    QVERIFY(emptyIds.isEmpty());
}

void TestMapperRegistry::testDuplicateMapperRegistration()
{
    MapperConfig config1 = createTestMapperConfig("TestMapper");
    MapperConfig config2 = createTestMapperConfig("TestMapper");
    
    registry->registerMapper("TestMapper", config1);
    
    // 尝试注册重复的Mapper应该抛出异常
    QVERIFY_EXCEPTION_THROWN(
        registry->registerMapper("TestMapper", config2),
        MappingException
    );
}

void TestMapperRegistry::testGetNonExistentMapper()
{
    // 获取不存在的Mapper配置应该抛出异常
    QVERIFY_EXCEPTION_THROWN(
        registry->getMapperConfig("NonExistentMapper"),
        MappingException
    );
}

void TestMapperRegistry::testValidateInvalidMapper()
{
    // 创建没有语句的无效Mapper
    MapperConfig invalidConfig;
    invalidConfig.namespace_ = "InvalidMapper";
    invalidConfig.xmlPath = ":/mappers/Invalid.xml";
    // statements为空
    
    registry->registerMapper("InvalidMapper", invalidConfig);
    
    QVERIFY(!registry->validateMapper("InvalidMapper"));
}

MapperConfig TestMapperRegistry::createTestMapperConfig(const QString& namespace_)
{
    MapperConfig config;
    config.namespace_ = namespace_;
    config.xmlPath = QString(":/mappers/%1.xml").arg(namespace_);
    
    // 添加一些测试语句
    StatementConfig selectStmt = createTestStatementConfig("selectUser", StatementType::SELECT);
    StatementConfig insertStmt = createTestStatementConfig("insertUser", StatementType::INSERT);
    StatementConfig updateStmt = createTestStatementConfig("updateUser", StatementType::UPDATE);
    StatementConfig deleteStmt = createTestStatementConfig("deleteUser", StatementType::DELETE);
    
    config.statements["selectUser"] = selectStmt;
    config.statements["insertUser"] = insertStmt;
    config.statements["updateUser"] = updateStmt;
    config.statements["deleteUser"] = deleteStmt;
    
    // 添加结果映射
    config.resultMaps["UserResultMap"] = "User";
    
    return config;
}

StatementConfig TestMapperRegistry::createTestStatementConfig(const QString& id, StatementType type)
{
    StatementConfig config;
    config.id = id;
    config.type = type;
    
    switch (type) {
        case StatementType::SELECT:
            config.sql = "SELECT * FROM users WHERE id = #{id}";
            config.resultType = "User";
            break;
        case StatementType::INSERT:
            config.sql = "INSERT INTO users (name, email) VALUES (#{name}, #{email})";
            break;
        case StatementType::UPDATE:
            config.sql = "UPDATE users SET name = #{name}, email = #{email} WHERE id = #{id}";
            break;
        case StatementType::DELETE:
            config.sql = "DELETE FROM users WHERE id = #{id}";
            break;
        case StatementType::DDL:
            config.sql = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100), email VARCHAR(100))";
            break;
    }
    
    config.parameterType = "QVariantMap";
    config.useCache = false;
    
    return config;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestMapperRegistry test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_mapperregistry_test.moc"