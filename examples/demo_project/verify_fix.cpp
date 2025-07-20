#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatishelper.h>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== 验证QtMyBatisORM修复效果 ===";
    
    // 初始化
    if (!QtMyBatisHelper::initialize(":/resources/config/database.json")) {
        qCritical() << "初始化失败";
        return 1;
    }
    
    qDebug() << "\n1. 测试COUNT查询（修复前会返回空值）:";
    QVariant count = QtMyBatisHelper::selectOne("Student.count");
    qDebug() << "   Student.count 结果:" << count;
    qDebug() << "   类型:" << count.typeName();
    qDebug() << "   整数值:" << count.toInt();
    
    if (count.isValid() && count.toInt() > 0) {
        qDebug() << "   ✅ COUNT查询修复成功！";
    } else {
        qDebug() << "   ❌ COUNT查询仍有问题";
    }
    
    qDebug() << "\n2. 测试单记录查询:";
    QVariantMap params;
    params["arg1"] = "2021001";
    QVariant student = QtMyBatisHelper::selectOne("Student.findByStudentNumber", params);
    qDebug() << "   findByStudentNumber 结果:" << student;
    qDebug() << "   类型:" << student.typeName();
    
    if (student.isValid() && student.toMap().contains("name")) {
        qDebug() << "   ✅ 单记录查询修复成功！";
        qDebug() << "   学生姓名:" << student.toMap()["name"].toString();
    } else {
        qDebug() << "   ❌ 单记录查询仍有问题";
    }
    
    qDebug() << "\n3. 测试列表查询（对比）:";
    QVariantList students = QtMyBatisHelper::selectList("Student.findAll");
    qDebug() << "   findAll 结果数量:" << students.size();
    
    if (students.size() > 0) {
        qDebug() << "   ✅ 列表查询正常工作";
    } else {
        qDebug() << "   ❌ 列表查询也有问题";
    }
    
    QtMyBatisHelper::shutdown();
    
    qDebug() << "\n=== 测试完成 ===";
    return 0;
} 