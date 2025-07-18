#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QElapsedTimer>

#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/DataModels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestMultiDatabaseIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // SQLite测试
    void testSQLiteBasicOperations();
    void testSQLiteTransactions();
    void testSQLiteMapperUsage();
    
    // MySQL测试（如果可用）
    void testMySQLConnection();
    void testMySQLBasicOperations();
    void testMySQLTransactions();
    void testMySQLMapperUsage();
    
    // 跨数据库测试
    void testCrossDatabaseCompatibility();
    void testDatabaseSpecificFeatures();
    
    // 多数据库连接测试
    void testMultipleConnections();
    void testConnectionSwitching();

private:
    void setupSQLiteDatabase();
    void setupMySQLDatabase();
    void setupXmlMappers();
    bool isMySQLAvailable();
    
    QTemporaryDir m_tempDir;
    QString m_sqliteDbPath;
    QString m_sqliteConfigPath;
    QString m_mysqlConfigPath;
    QStringList m_mapperPaths;
    
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> m_sqliteOrm;
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> m_mysqlOrm;
    
    bool m_mysqlAvailable;
};

void TestMultiDatabaseIntegration::initTestCase()
{
    // 创建临时目录用于测试文件
    QVERIFY(m_tempDir.isValid());
    
    // 设置SQLite数据库路径
    m_sqliteDbPath = m_tempDir.path() + "/test_multi_db.db";
    
    // 创建配置文件
    m_sqliteConfigPath = m_tempDir.path() + "/sqlite_config.json";
    m_mysqlConfigPath = m_tempDir.path() + "/mysql_config.json";
    
    // 创建XML映射文件
    setupXmlMappers();
    
    // 设置SQLite数据库
    setupSQLiteDatabase();
    
    // 检查MySQL是否可用
    m_mysqlAvailable = isMySQLAvailable();
    if (m_mysqlAvailable) {
        setupMySQLDatabase();
    } else {
        qWarning() << "MySQL不可用，跳过MySQL相关测试";
    }
}

void TestMultiDatabaseIntegration::cleanupTestCase()
{
    m_sqliteOrm.reset();
    m_mysqlOrm.reset();
}

void TestMultiDatabaseIntegration::init()
{
    // 每个测试前的准备工作
}

void TestMultiDatabaseIntegration::cleanup()
{
    // 每个测试后的清理工作
}

bool TestMultiDatabaseIntegration::isMySQLAvailable()
{
    // 检查MySQL驱动是否可用
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        return false;
    }
    
    // 尝试连接到MySQL
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "mysql_test_connection");
    db.setHostName("localhost");
    db.setPort(3306);
    db.setUserName("root"); // 使用默认用户名，实际环境中应该使用专用测试用户
    db.setPassword(""); // 空密码，实际环境中应该使用安全密码
    db.setDatabaseName("test"); // 使用test数据库，实际环境中应该使用专用测试数据库
    
    bool connected = db.open();
    
    if (connected) {
        db.close();
    }
    
    QSqlDatabase::removeDatabase("mysql_test_connection");
    
    return connected;
}

void TestMultiDatabaseIntegration::setupSQLiteDatabase()
{
    // 创建SQLite配置文件
    QFile configFile(m_sqliteConfigPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QJsonObject config;
    config["driverName"] = "QSQLITE";
    config["databaseName"] = m_sqliteDbPath;
    config["cacheEnabled"] = true;
    config["maxCacheSize"] = 100;
    config["cacheExpireTime"] = 300;
    config["maxConnections"] = 5;
    config["minConnections"] = 1;
    config["maxIdleTime"] = 60;
    
    QJsonDocument doc(config);
    configFile.write(doc.toJson());
    configFile.close();
    
    // 创建ORM实例
    m_sqliteOrm = QtMyBatisORM::create(m_sqliteConfigPath, m_mapperPaths);
    QVERIFY(m_sqliteOrm != nullptr);
    
    // 创建测试表
    auto session = m_sqliteOrm->openSession();
    
    // 创建用户表
    session->execute(R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            created_at TEXT NOT NULL
        )
    )");
    
    // 创建文章表
    session->execute(R"(
        CREATE TABLE articles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            user_id INTEGER NOT NULL,
            published_at TEXT,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )");
    
    // 插入测试数据
    session->beginTransaction();
    
    try {
        // 插入用户
        for (int i = 1; i <= 5; ++i) {
            QVariantMap user;
            user["username"] = QString("user%1").arg(i);
            user["email"] = QString("user%1@example.com").arg(i);
            user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            session->execute(
                "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
                user
            );
        }
        
        // 插入文章
        for (int i = 1; i <= 10; ++i) {
            QVariantMap article;
            article["title"] = QString("Article %1").arg(i);
            article["content"] = QString("Content for article %1").arg(i);
            article["user_id"] = (i % 5) + 1;
            article["published_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            session->execute(
                "INSERT INTO articles (title, content, user_id, published_at) "
                "VALUES (:title, :content, :user_id, :published_at)",
                article
            );
        }
        
        session->commit();
    } catch (...) {
        session->rollback();
        throw;
    }
    
    m_sqliteOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::setupMySQLDatabase()
{
    if (!m_mysqlAvailable) {
        return;
    }
    
    // 创建MySQL配置文件
    QFile configFile(m_mysqlConfigPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QJsonObject config;
    config["driverName"] = "QMYSQL";
    config["hostName"] = "localhost";
    config["port"] = 3306;
    config["databaseName"] = "test";
    config["userName"] = "root";
    config["password"] = "";
    config["cacheEnabled"] = true;
    config["maxCacheSize"] = 100;
    config["cacheExpireTime"] = 300;
    config["maxConnections"] = 5;
    config["minConnections"] = 1;
    config["maxIdleTime"] = 60;
    
    QJsonDocument doc(config);
    configFile.write(doc.toJson());
    configFile.close();
    
    // 创建ORM实例
    m_mysqlOrm = QtMyBatisORM::create(m_mysqlConfigPath, m_mapperPaths);
    QVERIFY(m_mysqlOrm != nullptr);
    
    // 创建测试表
    auto session = m_mysqlOrm->openSession();
    
    // 删除已存在的表
    try {
        session->execute("DROP TABLE IF EXISTS articles");
        session->execute("DROP TABLE IF EXISTS users");
    } catch (...) {
        // 忽略错误
    }
    
    // 创建用户表
    session->execute(R"(
        CREATE TABLE users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(50) NOT NULL,
            email VARCHAR(100) UNIQUE NOT NULL,
            created_at DATETIME NOT NULL
        )
    )");
    
    // 创建文章表
    session->execute(R"(
        CREATE TABLE articles (
            id INT AUTO_INCREMENT PRIMARY KEY,
            title VARCHAR(200) NOT NULL,
            content TEXT NOT NULL,
            user_id INT NOT NULL,
            published_at DATETIME,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )");
    
    // 插入测试数据
    session->beginTransaction();
    
    try {
        // 插入用户
        for (int i = 1; i <= 5; ++i) {
            QVariantMap user;
            user["username"] = QString("user%1").arg(i);
            user["email"] = QString("user%1@example.com").arg(i);
            user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            session->execute(
                "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
                user
            );
        }
        
        // 插入文章
        for (int i = 1; i <= 10; ++i) {
            QVariantMap article;
            article["title"] = QString("Article %1").arg(i);
            article["content"] = QString("Content for article %1").arg(i);
            article["user_id"] = (i % 5) + 1;
            article["published_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            session->execute(
                "INSERT INTO articles (title, content, user_id, published_at) "
                "VALUES (:title, :content, :user_id, :published_at)",
                article
            );
        }
        
        session->commit();
    } catch (...) {
        session->rollback();
        throw;
    }
    
    m_mysqlOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::setupXmlMappers()
{
    // 创建用户Mapper XML
    QString userMapperPath = m_tempDir.path() + "/user_mapper.xml";
    QFile userMapperFile(userMapperPath);
    QVERIFY(userMapperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QString userMapperXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="UserMapper">
    <select id="findAll" resultType="User">
        SELECT * FROM users
    </select>
    
    <select id="findById" parameterType="int" resultType="User">
        SELECT * FROM users WHERE id = :id
    </select>
    
    <select id="findByUsername" parameterType="string" resultType="User">
        SELECT * FROM users WHERE username = :username
    </select>
    
    <insert id="insert" parameterType="User">
        INSERT INTO users (username, email, created_at) 
        VALUES (:username, :email, :created_at)
    </insert>
    
    <update id="update" parameterType="User">
        UPDATE users SET username = :username, email = :email 
        WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM users WHERE id = :id
    </delete>
</mapper>)";
    
    userMapperFile.write(userMapperXml.toUtf8());
    userMapperFile.close();
    
    // 创建文章Mapper XML
    QString articleMapperPath = m_tempDir.path() + "/article_mapper.xml";
    QFile articleMapperFile(articleMapperPath);
    QVERIFY(articleMapperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QString articleMapperXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="ArticleMapper">
    <select id="findAll" resultType="Article">
        SELECT * FROM articles
    </select>
    
    <select id="findById" parameterType="int" resultType="Article">
        SELECT * FROM articles WHERE id = :id
    </select>
    
    <select id="findByUserId" parameterType="int" resultType="Article">
        SELECT * FROM articles WHERE user_id = :userId
    </select>
    
    <insert id="insert" parameterType="Article">
        INSERT INTO articles (title, content, user_id, published_at) 
        VALUES (:title, :content, :userId, :publishedAt)
    </insert>
    
    <update id="update" parameterType="Article">
        UPDATE articles SET title = :title, content = :content 
        WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM articles WHERE id = :id
    </delete>
    
    <!-- 数据库特定的查询 -->
    <select id="findWithUserInfo" resultType="ArticleWithUser">
        SELECT a.*, u.username, u.email 
        FROM articles a 
        JOIN users u ON a.user_id = u.id
    </select>
</mapper>)";
    
    articleMapperFile.write(articleMapperXml.toUtf8());
    articleMapperFile.close();
    
    m_mapperPaths << userMapperPath << articleMapperPath;
}

void TestMultiDatabaseIntegration::testSQLiteBasicOperations()
{
    auto session = m_sqliteOrm->openSession();
    
    // 测试查询
    QVariantList users = session->selectList("SELECT * FROM users");
    QCOMPARE(users.size(), 5);
    
    QVariantList articles = session->selectList("SELECT * FROM articles");
    QCOMPARE(articles.size(), 10);
    
    // 测试插入
    QVariantMap newUser;
    newUser["username"] = "sqlite_user";
    newUser["email"] = "sqlite_user@example.com";
    newUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    session->execute(
        "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
        newUser
    );
    
    // 验证插入成功
    users = session->selectList("SELECT * FROM users");
    QCOMPARE(users.size(), 6);
    
    // 测试更新
    session->execute(
        "UPDATE users SET username = 'updated_sqlite_user' WHERE email = 'sqlite_user@example.com'"
    );
    
    // 验证更新成功
    QVariantMap params;
    params["email"] = "sqlite_user@example.com";
    QVariant user = session->selectOne("SELECT * FROM users WHERE email = :email", params);
    QVERIFY(!user.isNull());
    QVariantMap userMap = user.toMap();
    QCOMPARE(userMap["username"].toString(), QString("updated_sqlite_user"));
    
    // 测试删除
    session->execute(
        "DELETE FROM users WHERE email = 'sqlite_user@example.com'"
    );
    
    // 验证删除成功
    user = session->selectOne("SELECT * FROM users WHERE email = 'sqlite_user@example.com'");
    QVERIFY(user.isNull());
    
    m_sqliteOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testSQLiteTransactions()
{
    auto session = m_sqliteOrm->openSession();
    
    // 测试成功的事务
    session->beginTransaction();
    
    try {
        QVariantMap user;
        user["username"] = "transaction_user";
        user["email"] = "transaction_user@example.com";
        user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
            user
        );
        
        QVariantMap article;
        article["title"] = "Transaction Article";
        article["content"] = "This article was created in a transaction";
        article["user_id"] = session->selectOne("SELECT id FROM users WHERE email = 'transaction_user@example.com'").toMap()["id"];
        article["published_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO articles (title, content, user_id, published_at) "
            "VALUES (:title, :content, :user_id, :published_at)",
            article
        );
        
        session->commit();
    } catch (...) {
        session->rollback();
        QFAIL("事务应该成功提交");
    }
    
    // 验证事务提交成功
    QVariant user = session->selectOne("SELECT * FROM users WHERE email = 'transaction_user@example.com'");
    QVERIFY(!user.isNull());
    
    QVariantMap params;
    params["title"] = "Transaction Article";
    QVariant article = session->selectOne("SELECT * FROM articles WHERE title = :title", params);
    QVERIFY(!article.isNull());
    
    // 测试回滚的事务
    session->beginTransaction();
    
    try {
        QVariantMap user;
        user["username"] = "rollback_user";
        user["email"] = "rollback_user@example.com";
        user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
            user
        );
        
        // 手动回滚
        session->rollback();
    } catch (...) {
        session->rollback();
        QFAIL("不应该抛出异常");
    }
    
    // 验证事务回滚成功
    user = session->selectOne("SELECT * FROM users WHERE email = 'rollback_user@example.com'");
    QVERIFY(user.isNull());
    
    m_sqliteOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testSQLiteMapperUsage()
{
    auto session = m_sqliteOrm->openSession();
    
    // 测试查询映射
    QVariantList users = session->selectList("UserMapper.findAll");
    QVERIFY(users.size() > 0);
    
    QVariantList articles = session->selectList("ArticleMapper.findAll");
    QVERIFY(articles.size() > 0);
    
    // 测试参数化查询
    QVariantMap params;
    params["id"] = 1;
    QVariant user = session->selectOne("UserMapper.findById", params);
    QVERIFY(!user.isNull());
    
    // 测试插入
    QVariantMap newUser;
    newUser["username"] = "mapper_user";
    newUser["email"] = "mapper_user@example.com";
    newUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    int userId = session->insert("UserMapper.insert", newUser);
    QVERIFY(userId > 0);
    
    // 测试更新
    QVariantMap updateUser;
    updateUser["id"] = userId;
    updateUser["username"] = "updated_mapper_user";
    updateUser["email"] = "mapper_user@example.com";
    
    int updateResult = session->update("UserMapper.update", updateUser);
    QCOMPARE(updateResult, 1);
    
    // 验证更新成功
    params["id"] = userId;
    user = session->selectOne("UserMapper.findById", params);
    QVERIFY(!user.isNull());
    QVariantMap userMap = user.toMap();
    QCOMPARE(userMap["username"].toString(), QString("updated_mapper_user"));
    
    // 测试删除
    int deleteResult = session->deleteOne("UserMapper.deleteById", params);
    QCOMPARE(deleteResult, 1);
    
    // 验证删除成功
    user = session->selectOne("UserMapper.findById", params);
    QVERIFY(user.isNull());
    
    // 测试关联查询
    QVariantList articlesWithUsers = session->selectList("ArticleMapper.findWithUserInfo");
    QVERIFY(articlesWithUsers.size() > 0);
    
    // 验证关联查询结果
    QVariantMap articleWithUser = articlesWithUsers[0].toMap();
    QVERIFY(articleWithUser.contains("title"));
    QVERIFY(articleWithUser.contains("username"));
    QVERIFY(articleWithUser.contains("email"));
    
    m_sqliteOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testMySQLConnection()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    QVERIFY(m_mysqlOrm != nullptr);
    
    auto session = m_mysqlOrm->openSession();
    QVERIFY(session != nullptr);
    
    // 测试简单查询
    QVariantList users = session->selectList("SELECT * FROM users");
    QCOMPARE(users.size(), 5);
    
    m_mysqlOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testMySQLBasicOperations()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    auto session = m_mysqlOrm->openSession();
    
    // 测试查询
    QVariantList users = session->selectList("SELECT * FROM users");
    QCOMPARE(users.size(), 5);
    
    QVariantList articles = session->selectList("SELECT * FROM articles");
    QCOMPARE(articles.size(), 10);
    
    // 测试插入
    QVariantMap newUser;
    newUser["username"] = "mysql_user";
    newUser["email"] = "mysql_user@example.com";
    newUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    session->execute(
        "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
        newUser
    );
    
    // 验证插入成功
    users = session->selectList("SELECT * FROM users");
    QCOMPARE(users.size(), 6);
    
    // 测试更新
    session->execute(
        "UPDATE users SET username = 'updated_mysql_user' WHERE email = 'mysql_user@example.com'"
    );
    
    // 验证更新成功
    QVariantMap params;
    params["email"] = "mysql_user@example.com";
    QVariant user = session->selectOne("SELECT * FROM users WHERE email = :email", params);
    QVERIFY(!user.isNull());
    QVariantMap userMap = user.toMap();
    QCOMPARE(userMap["username"].toString(), QString("updated_mysql_user"));
    
    // 测试删除
    session->execute(
        "DELETE FROM users WHERE email = 'mysql_user@example.com'"
    );
    
    // 验证删除成功
    user = session->selectOne("SELECT * FROM users WHERE email = 'mysql_user@example.com'");
    QVERIFY(user.isNull());
    
    m_mysqlOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testMySQLTransactions()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    auto session = m_mysqlOrm->openSession();
    
    // 测试成功的事务
    session->beginTransaction();
    
    try {
        QVariantMap user;
        user["username"] = "mysql_transaction_user";
        user["email"] = "mysql_transaction_user@example.com";
        user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
            user
        );
        
        QVariantMap article;
        article["title"] = "MySQL Transaction Article";
        article["content"] = "This article was created in a MySQL transaction";
        article["user_id"] = session->selectOne("SELECT id FROM users WHERE email = 'mysql_transaction_user@example.com'").toMap()["id"];
        article["published_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO articles (title, content, user_id, published_at) "
            "VALUES (:title, :content, :user_id, :published_at)",
            article
        );
        
        session->commit();
    } catch (...) {
        session->rollback();
        QFAIL("事务应该成功提交");
    }
    
    // 验证事务提交成功
    QVariant user = session->selectOne("SELECT * FROM users WHERE email = 'mysql_transaction_user@example.com'");
    QVERIFY(!user.isNull());
    
    QVariantMap params;
    params["title"] = "MySQL Transaction Article";
    QVariant article = session->selectOne("SELECT * FROM articles WHERE title = :title", params);
    QVERIFY(!article.isNull());
    
    // 测试回滚的事务
    session->beginTransaction();
    
    try {
        QVariantMap user;
        user["username"] = "mysql_rollback_user";
        user["email"] = "mysql_rollback_user@example.com";
        user["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        session->execute(
            "INSERT INTO users (username, email, created_at) VALUES (:username, :email, :created_at)",
            user
        );
        
        // 手动回滚
        session->rollback();
    } catch (...) {
        session->rollback();
        QFAIL("不应该抛出异常");
    }
    
    // 验证事务回滚成功
    user = session->selectOne("SELECT * FROM users WHERE email = 'mysql_rollback_user@example.com'");
    QVERIFY(user.isNull());
    
    m_mysqlOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testMySQLMapperUsage()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    auto session = m_mysqlOrm->openSession();
    
    // 测试查询映射
    QVariantList users = session->selectList("UserMapper.findAll");
    QVERIFY(users.size() > 0);
    
    QVariantList articles = session->selectList("ArticleMapper.findAll");
    QVERIFY(articles.size() > 0);
    
    // 测试参数化查询
    QVariantMap params;
    params["id"] = 1;
    QVariant user = session->selectOne("UserMapper.findById", params);
    QVERIFY(!user.isNull());
    
    // 测试插入
    QVariantMap newUser;
    newUser["username"] = "mysql_mapper_user";
    newUser["email"] = "mysql_mapper_user@example.com";
    newUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    int userId = session->insert("UserMapper.insert", newUser);
    QVERIFY(userId > 0);
    
    // 测试更新
    QVariantMap updateUser;
    updateUser["id"] = userId;
    updateUser["username"] = "updated_mysql_mapper_user";
    updateUser["email"] = "mysql_mapper_user@example.com";
    
    int updateResult = session->update("UserMapper.update", updateUser);
    QCOMPARE(updateResult, 1);
    
    // 验证更新成功
    params["id"] = userId;
    user = session->selectOne("UserMapper.findById", params);
    QVERIFY(!user.isNull());
    QVariantMap userMap = user.toMap();
    QCOMPARE(userMap["username"].toString(), QString("updated_mysql_mapper_user"));
    
    // 测试删除
    int deleteResult = session->deleteOne("UserMapper.deleteById", params);
    QCOMPARE(deleteResult, 1);
    
    // 验证删除成功
    user = session->selectOne("UserMapper.findById", params);
    QVERIFY(user.isNull());
    
    // 测试关联查询
    QVariantList articlesWithUsers = session->selectList("ArticleMapper.findWithUserInfo");
    QVERIFY(articlesWithUsers.size() > 0);
    
    // 验证关联查询结果
    QVariantMap articleWithUser = articlesWithUsers[0].toMap();
    QVERIFY(articleWithUser.contains("title"));
    QVERIFY(articleWithUser.contains("username"));
    QVERIFY(articleWithUser.contains("email"));
    
    m_mysqlOrm->closeSession(session);
}

void TestMultiDatabaseIntegration::testCrossDatabaseCompatibility()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    // 在SQLite中执行查询
    auto sqliteSession = m_sqliteOrm->openSession();
    QVariantList sqliteUsers = sqliteSession->selectList("UserMapper.findAll");
    QVariantList sqliteArticles = sqliteSession->selectList("ArticleMapper.findAll");
    m_sqliteOrm->closeSession(sqliteSession);
    
    // 在MySQL中执行相同的查询
    auto mysqlSession = m_mysqlOrm->openSession();
    QVariantList mysqlUsers = mysqlSession->selectList("UserMapper.findAll");
    QVariantList mysqlArticles = mysqlSession->selectList("ArticleMapper.findAll");
    m_mysqlOrm->closeSession(mysqlSession);
    
    // 验证结果集结构相同
    QCOMPARE(sqliteUsers.size(), mysqlUsers.size());
    QCOMPARE(sqliteArticles.size(), mysqlArticles.size());
    
    // 验证字段名相同
    QVariantMap sqliteUser = sqliteUsers[0].toMap();
    QVariantMap mysqlUser = mysqlUsers[0].toMap();
    
    QStringList sqliteUserKeys = sqliteUser.keys();
    QStringList mysqlUserKeys = mysqlUser.keys();
    
    // 排序键以便比较
    std::sort(sqliteUserKeys.begin(), sqliteUserKeys.end());
    std::sort(mysqlUserKeys.begin(), mysqlUserKeys.end());
    
    QCOMPARE(sqliteUserKeys, mysqlUserKeys);
    
    // 验证文章字段名相同
    QVariantMap sqliteArticle = sqliteArticles[0].toMap();
    QVariantMap mysqlArticle = mysqlArticles[0].toMap();
    
    QStringList sqliteArticleKeys = sqliteArticle.keys();
    QStringList mysqlArticleKeys = mysqlArticle.keys();
    
    // 排序键以便比较
    std::sort(sqliteArticleKeys.begin(), sqliteArticleKeys.end());
    std::sort(mysqlArticleKeys.begin(), mysqlArticleKeys.end());
    
    QCOMPARE(sqliteArticleKeys, mysqlArticleKeys);
}

void TestMultiDatabaseIntegration::testDatabaseSpecificFeatures()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    // SQLite特定功能测试
    auto sqliteSession = m_sqliteOrm->openSession();
    
    // 测试SQLite的ROWID
    QVariant sqliteResult = sqliteSession->selectOne("SELECT rowid FROM users LIMIT 1");
    QVERIFY(!sqliteResult.isNull());
    
    m_sqliteOrm->closeSession(sqliteSession);
    
    // MySQL特定功能测试
    auto mysqlSession = m_mysqlOrm->openSession();
    
    // 测试MySQL的LIMIT语法
    QVariantList mysqlResults = mysqlSession->selectList("SELECT * FROM users LIMIT 2, 3");
    QCOMPARE(mysqlResults.size(), 3);
    
    // 测试MySQL的函数
    QVariant mysqlResult = mysqlSession->selectOne("SELECT VERSION()");
    QVERIFY(!mysqlResult.isNull());
    
    m_mysqlOrm->closeSession(mysqlSession);
}

void TestMultiDatabaseIntegration::testMultipleConnections()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    // 同时打开多个SQLite连接
    QList<QSharedPointer<Session>> sqliteSessions;
    for (int i = 0; i < 5; ++i) {
        auto session = m_sqliteOrm->openSession();
        QVERIFY(session != nullptr);
        sqliteSessions.append(session);
    }
    
    // 同时打开多个MySQL连接
    QList<QSharedPointer<Session>> mysqlSessions;
    for (int i = 0; i < 5; ++i) {
        auto session = m_mysqlOrm->openSession();
        QVERIFY(session != nullptr);
        mysqlSessions.append(session);
    }
    
    // 在所有连接上执行查询
    for (auto& session : sqliteSessions) {
        QVariantList users = session->selectList("SELECT * FROM users");
        QVERIFY(users.size() > 0);
    }
    
    for (auto& session : mysqlSessions) {
        QVariantList users = session->selectList("SELECT * FROM users");
        QVERIFY(users.size() > 0);
    }
    
    // 关闭所有连接
    for (auto& session : sqliteSessions) {
        m_sqliteOrm->closeSession(session);
    }
    
    for (auto& session : mysqlSessions) {
        m_mysqlOrm->closeSession(session);
    }
}

void TestMultiDatabaseIntegration::testConnectionSwitching()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    // 创建会话
    auto sqliteSession = m_sqliteOrm->openSession();
    auto mysqlSession = m_mysqlOrm->openSession();
    
    // 在SQLite中插入数据
    QVariantMap sqliteUser;
    sqliteUser["username"] = "switching_test_sqlite";
    sqliteUser["email"] = "switching_test_sqlite@example.com";
    sqliteUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    int sqliteUserId = sqliteSession->insert("UserMapper.insert", sqliteUser);
    QVERIFY(sqliteUserId > 0);
    
    // 在MySQL中插入数据
    QVariantMap mysqlUser;
    mysqlUser["username"] = "switching_test_mysql";
    mysqlUser["email"] = "switching_test_mysql@example.com";
    mysqlUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    int mysqlUserId = mysqlSession->insert("UserMapper.insert", mysqlUser);
    QVERIFY(mysqlUserId > 0);
    
    // 验证SQLite中的数据
    QVariantMap params;
    params["username"] = "switching_test_sqlite";
    QVariant sqliteResult = sqliteSession->selectOne("UserMapper.findByUsername", params);
    QVERIFY(!sqliteResult.isNull());
    
    // 验证MySQL中的数据
    params["username"] = "switching_test_mysql";
    QVariant mysqlResult = mysqlSession->selectOne("UserMapper.findByUsername", params);
    QVERIFY(!mysqlResult.isNull());
    
    // 验证数据库隔离性
    params["username"] = "switching_test_sqlite";
    mysqlResult = mysqlSession->selectOne("UserMapper.findByUsername", params);
    QVERIFY(mysqlResult.isNull()); // MySQL中不应该有SQLite的数据
    
    params["username"] = "switching_test_mysql";
    sqliteResult = sqliteSession->selectOne("UserMapper.findByUsername", params);
    QVERIFY(sqliteResult.isNull()); // SQLite中不应该有MySQL的数据
    
    // 关闭会话
    m_sqliteOrm->closeSession(sqliteSession);
    m_mysqlOrm->closeSession(mysqlSession);
}

