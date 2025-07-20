#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "测试 COUNT 查询问题";
    
    // 初始化
    if (!QtMyBatisHelper::initialize(":/resources/config/database.json")) {
        qCritical() << "初始化失败";
        return 1;
    }
    
    QtMyBatisHelper::enableDebugMode(true);
    
    try {
        // 测试1: 直接执行 SQL
        qDebug() << "\n=== 测试1: 直接SQL ===";
        int directResult = QtMyBatisHelper::execute("SELECT COUNT(*) FROM students WHERE status = 'ACTIVE'");
        qDebug() << "Direct SQL result:" << directResult;
        
        // 测试2: 使用映射的COUNT查询
        qDebug() << "\n=== 测试2: 映射的COUNT查询 ===";
        QVariant countResult = QtMyBatisHelper::selectOne("Student.count");
        qDebug() << "Mapped count result:" << countResult;
        qDebug() << "Result type:" << countResult.typeName();
        qDebug() << "Is valid:" << countResult.isValid();
        qDebug() << "Is null:" << countResult.isNull();
        qDebug() << "To int:" << countResult.toInt();
        qDebug() << "To string:" << countResult.toString();
        
        // 测试3: 简单的查询
        qDebug() << "\n=== 测试3: 简单查询 ===";
        QVariantList allStudents = QtMyBatisHelper::selectList("Student.findAll");
        qDebug() << "Found students:" << allStudents.size();
        
    } catch (const QtMyBatisException& e) {
        qCritical() << "Exception:" << e.message();
    }
    
    QtMyBatisHelper::shutdown();
    return 0;
} 