#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>
#include <QCoreApplication>
#include <QDebug>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== 简单验证修复效果 ===";
    
    try {
        if (!QtMyBatisHelper::initialize(":/resources/config/database.json")) {
            qDebug() << "初始化失败";
            return 1;
        }
        
        qDebug() << "测试Student.count:";
        QVariant count = QtMyBatisHelper::selectOne("Student.count");
        qDebug() << "结果:" << count;
        qDebug() << "类型:" << count.typeName();
        qDebug() << "有效?" << count.isValid();
        qDebug() << "空?" << count.isNull();
        qDebug() << "整数:" << count.toInt();
        
        if (count.isValid() && count.toInt() == 6) {
            qDebug() << "✅ 修复成功! COUNT查询返回正确的值";
            return 0;
        } else {
            qDebug() << "❌ 修复失败! COUNT查询仍返回空或错误值";
            return 1;
        }
        
    } catch (const QtMyBatisException& e) {
        qDebug() << "异常:" << e.message();
        return 1;
    }
} 