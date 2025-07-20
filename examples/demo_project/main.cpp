#include <QCoreApplication>
#include <QDebug>
#include <QDate>
#include <QThread>

#include "service/educationservice.h"
#include "models/student.h"
#include "models/course.h"

/**
 * QtMyBatisORM 演示程序
 * 
 * 这个程序展示了如何使用QtMyBatisORM进行完整的数据库操作，包括：
 * 1. 数据库初始化和表创建
 * 2. 基础CRUD操作
 * 3. 复杂查询和统计
 * 4. 事务管理
 * 5. 业务逻辑封装
 */

void printSeparator(const QString& title) {
    qDebug() << "\n" << QString("=").repeated(60);
    qDebug() << QString("  %1").arg(title);
    qDebug() << QString("=").repeated(60);
}

void demonstrateStudentOperations(EducationService* service) {
    printSeparator("学生管理演示");
    
    // 1. 查询所有学生
    qDebug() << "\n1. 查询所有学生:";
    QList<Student> students = service->getAllStudents();
    qDebug() << "总共有" << students.size() << "名学生";
    
    for (const Student& student : students) {
        qDebug() << QString("  - %1 (%2) - %3 - %4年级")
                    .arg(student.name())
                    .arg(student.studentNumber())
                    .arg(student.major())
                    .arg(student.grade());
    }
    
    // 2. 添加新学生
    qDebug() << "\n2. 添加新学生:";
    Student newStudent("2024002", "王小红", Student::Gender::FEMALE); // 使用不同的学号避免重复
    newStudent.setMajor("软件工程");
    newStudent.setGrade(2024);
    newStudent.setBirthDate(QDate(2005, 8, 20));
    newStudent.setPhone("13987654321");
    newStudent.setEmail("wangxiaohong@example.com");
    newStudent.setEnrollmentDate(QDate::currentDate());
    
    // 检查学生是否已存在
    Student existingStudent = service->getStudentByNumber(newStudent.studentNumber());
    if (existingStudent.isValid()) {
        qDebug() << "学生已存在:" << existingStudent.displayName();
    } else {
        if (service->addStudent(newStudent)) {
            qDebug() << "成功添加学生:" << newStudent.displayName();
        } else {
            qDebug() << "添加学生失败";
        }
    }
    
    // 3. 按专业搜索学生
    qDebug() << "\n3. 搜索计算机相关专业学生:";
    QList<Student> csStudents = service->searchStudents("计算机");
    qDebug() << "找到" << csStudents.size() << "名计算机相关专业学生";
    
    for (const Student& student : csStudents) {
        qDebug() << QString("  - %1").arg(student.displayName());
    }
}

void demonstrateCourseOperations(EducationService* service) {
    printSeparator("课程管理演示");
    
    // 1. 查询所有课程
    qDebug() << "\n1. 查询所有课程:";
    QList<Course> courses = service->getAllCourses();
    qDebug() << "总共有" << courses.size() << "门课程";
    
    for (const Course& course : courses) {
        qDebug() << QString("  - %1 - %2 (%3学分) - %4")
                    .arg(course.courseCode())
                    .arg(course.name())
                    .arg(course.credits())
                    .arg(course.teacher());
    }
    
    // 2. 添加新课程
    qDebug() << "\n2. 添加新课程:";
    Course newCourse("CS402", "深度学习进阶"); // 使用不同的课程代码避免重复
    newCourse.setDescription("深度学习算法与神经网络应用");
    newCourse.setCredits(4);
    newCourse.setHours(64);
    newCourse.setTeacher("李教授");
    newCourse.setDepartment("计算机学院");
    newCourse.setSemester("2024-2");
    newCourse.setMaxStudents(30);
    
    // 检查课程是否已存在
    Course existingCourse = service->getCourseByCode(newCourse.courseCode());
    if (existingCourse.isValid()) {
        qDebug() << "课程已存在:" << existingCourse.displayName();
    } else {
        if (service->addCourse(newCourse)) {
            qDebug() << "成功添加课程:" << newCourse.displayName();
        } else {
            qDebug() << "添加课程失败";
        }
    }
    
    // 3. 按院系搜索课程
    qDebug() << "\n3. 搜索计算机学院课程:";
    QList<Course> csCourses = service->searchCourses("计算机学院");
    qDebug() << "找到" << csCourses.size() << "门计算机学院课程";
    
    for (const Course& course : csCourses) {
        qDebug() << QString("  - %1").arg(course.displayName());
    }
}

void demonstrateEnrollmentOperations(EducationService* service) {
    printSeparator("选课管理演示");
    
    // 1. 学生选课
    qDebug() << "\n1. 学生选课演示:";
    
    // 让第一个学生选择第一门课程
    QList<Student> students = service->getAllStudents();
    QList<Course> courses = service->getAllCourses();
    
    if (!students.isEmpty() && !courses.isEmpty()) {
        QString studentNumber = students.first().studentNumber();
        QString courseCode = courses.first().courseCode();
        
        qDebug() << QString("学生 %1 选择课程 %2").arg(studentNumber, courseCode);
        
        if (service->enrollStudent(studentNumber, courseCode)) {
            qDebug() << "选课成功";
        } else {
            qDebug() << "选课失败（可能已经选过）";
        }
    }
    
    // 2. 更新成绩
    qDebug() << "\n2. 更新成绩演示:";
    if (!students.isEmpty() && !courses.isEmpty()) {
        QString studentNumber = students.first().studentNumber();
        QString courseCode = courses.first().courseCode();
        double grade = 85.5;
        
        qDebug() << QString("为学生 %1 在课程 %2 中设置成绩 %3")
                    .arg(studentNumber, courseCode).arg(grade);
        
        if (service->updateGrade(studentNumber, courseCode, grade)) {
            qDebug() << "成绩更新成功";
        } else {
            qDebug() << "成绩更新失败";
        }
    }
}

void demonstrateStatistics(EducationService* service) {
    printSeparator("统计分析演示");
    
    // 获取整体统计信息
    EducationService::Statistics stats = service->getOverallStatistics();
    
    qDebug() << "\n整体统计信息:";
    qDebug() << QString("  总学生数: %1").arg(stats.totalStudents);
    qDebug() << QString("  总课程数: %1").arg(stats.totalCourses);
    qDebug() << QString("  总选课数: %1").arg(stats.totalEnrollments);
    qDebug() << QString("  平均GPA: %1").arg(stats.averageGPA, 0, 'f', 2);
    qDebug() << QString("  热门专业: %1").arg(stats.mostPopularMajor);
    qDebug() << QString("  热门课程: %1").arg(stats.mostPopularCourse);
}

void demonstrateAdvancedFeatures(EducationService* service) {
    printSeparator("高级功能演示");
    
    // 1. 学生转专业（事务操作）
    qDebug() << "\n1. 学生转专业演示:";
    QList<Student> students = service->getAllStudents();
    if (!students.isEmpty()) {
        QString studentNumber = students.last().studentNumber();
        QString newMajor = "人工智能";
        int newGrade = 2024;
        
        qDebug() << QString("学生 %1 转到专业 %2, 年级 %3")
                    .arg(studentNumber, newMajor).arg(newGrade);
        
        if (service->transferStudent(studentNumber, newMajor, newGrade)) {
            qDebug() << "转专业成功";
        } else {
            qDebug() << "转专业失败";
        }
    }
    
    // 2. 数据验证演示
    qDebug() << "\n2. 数据验证演示:";
    Student invalidStudent;
    invalidStudent.setStudentNumber(""); // 无效的学号
    invalidStudent.setName(""); // 无效的姓名
    invalidStudent.setEmail("invalid-email"); // 无效的邮箱
    
    QString validationError = service->validateStudent(invalidStudent);
    if (!validationError.isEmpty()) {
        qDebug() << "数据验证失败:" << validationError;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("QtMyBatisORM Demo");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QtMyBatisORM Team");
    
    qDebug() << "QtMyBatisORM 演示程序启动";
    qDebug() << "版本: 1.0.0";
    qDebug() << "这个程序展示了QtMyBatisORM的完整功能和最佳实践";
    
    // 创建教育管理服务
    EducationService service;
    
    // 初始化服务
    if (!service.initialize()) {
        qCritical() << "教育管理服务初始化失败";
        return 1;
    }
    
    try {
        // 演示各种功能
        demonstrateStudentOperations(&service);
        demonstrateCourseOperations(&service);
        demonstrateEnrollmentOperations(&service);
        demonstrateStatistics(&service);
        demonstrateAdvancedFeatures(&service);
        
        printSeparator("演示完成");
        qDebug() << "\n✅ 所有演示都已完成!";
        qDebug() << "\n这个程序展示了QtMyBatisORM的主要功能:";
        qDebug() << "  • 一行初始化配置";
        qDebug() << "  • 统一的资源文件管理";
        qDebug() << "  • 简洁的静态API";
        qDebug() << "  • 完整的CRUD操作";
        qDebug() << "  • 事务管理";
        qDebug() << "  • 数据验证";
        qDebug() << "  • SQL调试日志";
        qDebug() << "  • 业务逻辑封装";
        
        qDebug() << "\n📚 更多信息请查看:";
        qDebug() << "  • 配置文件: resources/config/database.json";
        qDebug() << "  • SQL文件: resources/sql/*.sql";
        qDebug() << "  • 源代码: models/, dao/, service/";
        
    } catch (const std::exception& e) {
        qCritical() << "程序执行过程中发生错误:" << e.what();
        return 1;
    }
    
    // 清理资源
    service.shutdown();
    
    qDebug() << "\n程序即将退出...";
    
    return 0;
} 