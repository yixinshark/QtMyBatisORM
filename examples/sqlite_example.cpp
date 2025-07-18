#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QtMyBatisORM/qtmybatisorm.h>
#include <QtMyBatisORM/session.h>
#include <QtMyBatisORM/DataModels.h>

using namespace QtMyBatisORM;

// 定义一个简单的用户类
class User {
public:
    int id;
    QString name;
    QString email;
    
    User() : id(0) {}
    
    User(int id, const QString& name, const QString& email)
        : id(id), name(name), email(email) {}
    
    // 转换为QVariantMap，用于插入数据库
    QVariantMap toMap() const {
        QVariantMap map;
        map["id"] = id;
        map["name"] = name;
        map["email"] = email;
        return map;
    }
    
    // 从QVariantMap创建User对象，用于从数据库读取
    static User fromMap(const QVariantMap& map) {
        User user;
        user.id = map["id"].toInt();
        user.name = map["name"].toString();
        user.email = map["email"].toString();
        return user;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 创建一个SQLite数据库文件
    QString dbPath = QDir::currentPath() + "/example.db";
    QFile::remove(dbPath); // 删除可能存在的旧数据库文件
    
    // 创建ORM实例，使用SQLite数据库
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createSQLite(dbPath);
    
    if (!orm) {
        qCritical() << "Failed to create QtMyBatisORM instance";
        return 1;
    }
    
    qDebug() << "QtMyBatisORM initialized successfully with SQLite database at:" << dbPath;
    
    // 获取会话
    QSharedPointer<Session> session = orm->openSession();
    
    if (!session) {
        qCritical() << "Failed to open session";
        return 1;
    }
    
    qDebug() << "Session opened successfully";
    
    // 执行一些基本操作
    try {
        // 开始事务
        session->beginTransaction();
        
        // 创建表
        session->execute("CREATE TABLE users ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "name TEXT NOT NULL, "
                        "email TEXT NOT NULL UNIQUE)");
        qDebug() << "Table created successfully";
        
        // 插入多个用户
        QList<User> usersToInsert = {
            User(0, "John Doe", "john@example.com"),
            User(0, "Jane Smith", "jane@example.com"),
            User(0, "Bob Johnson", "bob@example.com")
        };
        
        for (const User& user : usersToInsert) {
            QVariantMap userMap = user.toMap();
            // 不包含ID，因为它是自动递增的
            userMap.remove("id");
            
            session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", userMap);
        }
        
        // 提交事务
        session->commit();
        qDebug() << "Users inserted successfully";
        
        // 查询所有用户
        QVariantList userVars = session->selectList("SELECT * FROM users");
        qDebug() << "Found" << userVars.size() << "users:";
        
        QList<User> users;
        for (const QVariant& userVar : userVars) {
            User user = User::fromMap(userVar.toMap());
            users.append(user);
            qDebug() << "User:" << user.id << user.name << user.email;
        }
        
        // 更新用户
        if (!users.isEmpty()) {
            User& firstUser = users[0];
            firstUser.email = "updated_" + firstUser.email;
            
            QVariantMap updateMap;
            updateMap["id"] = firstUser.id;
            updateMap["email"] = firstUser.email;
            
            session->execute("UPDATE users SET email = :email WHERE id = :id", updateMap);
            qDebug() << "User updated successfully";
            
            // 验证更新
            QVariantMap params;
            params["id"] = firstUser.id;
            QVariant updatedUserVar = session->selectOne("SELECT * FROM users WHERE id = :id", params);
            
            if (!updatedUserVar.isNull()) {
                User updatedUser = User::fromMap(updatedUserVar.toMap());
                qDebug() << "Updated user:" << updatedUser.id << updatedUser.name << updatedUser.email;
            }
        }
        
        // 删除用户
        if (users.size() > 1) {
            QVariantMap deleteParams;
            deleteParams["id"] = users[1].id;
            
            session->execute("DELETE FROM users WHERE id = :id", deleteParams);
            qDebug() << "User deleted successfully";
            
            // 验证删除
            QVariantList remainingUsers = session->selectList("SELECT * FROM users");
            qDebug() << "Remaining users:" << remainingUsers.size();
        }
        
    } catch (const QtMyBatisException& e) {
        qCritical() << "Error:" << e.message();
        
        // 如果发生错误，回滚事务
        if (session->isInTransaction()) {
            session->rollback();
            qDebug() << "Transaction rolled back";
        }
        
        return 1;
    }
    
    // 关闭会话
    orm->closeSession(session);
    qDebug() << "Session closed successfully";
    
    return 0;
}