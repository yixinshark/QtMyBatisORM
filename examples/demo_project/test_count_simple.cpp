#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== 简单COUNT查询测试 ===";
    
    // 初始化
    if (!QtMyBatisHelper::initialize(":/resources/config/database.json")) {
        qCritical() << "初始化失败";
        return 1;
    }
    
    QtMyBatisHelper::enableDebugMode(true);
    
    try {
        // 测试修复后的COUNT查询
        qDebug() << "\n--- 测试Student.count ---";
        QVariant countResult = QtMyBatisHelper::selectOne("Student.count");
        qDebug() << "COUNT结果:" << countResult;
        qDebug() << "结果类型:" << countResult.typeName();
        qDebug() << "是否有效:" << countResult.isValid();
        qDebug() << "转换为整数:" << countResult.toInt();
        
        // 测试是否成功
        if (countResult.isValid() && countResult.toInt() > 0) {
            qDebug() << "✅ COUNT查询修复成功！";
        } else {
            qDebug() << "❌ COUNT查询仍有问题";
        }
        
    } catch (const QtMyBatisException& e) {
        qCritical() << "异常:" << e.message();
    }
    
    QtMyBatisHelper::shutdown();
    return 0;
} 