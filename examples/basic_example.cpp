#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatisorm.h>
#include <QtMyBatisORM/session.h>
#include <QtMyBatisORM/DataModels.h>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 创建一个内存数据库的ORM实例
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createDefault();
    
    if (!orm) {
        qCritical() << "Failed to create QtMyBatisORM instance";
        return 1;
    }
    
    qDebug() << "QtMyBatisORM initialized successfully";
    
    // 获取会话
    QSharedPointer<Session> session = orm->openSession();
    
    if (!session) {
        qCritical() << "Failed to open session";
        return 1;
    }
    
    qDebug() << "Session opened successfully";
    
    // 执行一些基本操作
    try {
        // 创建表
        session->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
        qDebug() << "Table created successfully";
        
        // 插入数据
        QVariantMap user1;
        user1["name"] = "John Doe";
        user1["email"] = "john@example.com";
        
        session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", user1);
        qDebug() << "User inserted successfully";
        
        // 查询数据
        QVariantList users = session->selectList("SELECT * FROM users");
        qDebug() << "Found" << users.size() << "users";
        
        for (const QVariant& userVar : users) {
            QVariantMap user = userVar.toMap();
            qDebug() << "User:" << user["id"].toInt() << user["name"].toString() << user["email"].toString();
        }
    } catch (const QtMyBatisException& e) {
        qCritical() << "Error:" << e.message();
        return 1;
    }
    
    // 关闭会话
    orm->closeSession(session);
    qDebug() << "Session closed successfully";
    
    return 0;
}