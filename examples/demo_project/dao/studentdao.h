#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantMap>
#include "../models/student.h"

/**
 * 学生数据访问对象类
 * 
 * 这个类演示了如何使用QtMyBatisHelper进行数据库操作：
 * - 基础CRUD操作
 * - 条件查询和分页
 * - 复杂查询和统计
 * - 批量操作和事务
 * - 数据验证和错误处理
 */
class StudentDao : public QObject
{
    Q_OBJECT

public:
    explicit StudentDao(QObject* parent = nullptr);
    ~StudentDao() = default;

    // 基础CRUD操作
    QList<Student> findAll();
    Student findById(int id);
    Student findByStudentNumber(const QString& studentNumber);
    int count();
    bool insert(const Student& student);
    bool update(const Student& student);
    bool deleteById(int id);

    // 条件查询
    QList<Student> findByMajor(const QString& major);
    QList<Student> findByGrade(int grade);
    QList<Student> findByNamePattern(const QString& namePattern);

    // 分页查询
    QList<Student> findWithPagination(int limit, int offset);
    QList<Student> findByMajorWithPagination(const QString& major, int limit, int offset);

    // 高级查询
    QList<Student> findStudentsWithCourseCount();
    QList<Student> findStudentsInCourse(int courseId);
    QList<Student> findTopStudentsByGPA(double minGPA, int limit);

    // 批量操作
    bool batchInsert(const QList<Student>& students);
    bool batchUpdateGrade(const QStringList& studentNumbers, int grade);
    bool batchUpdateMajor(const QList<int>& studentIds, const QString& major);

    // 统计查询
    QVariantList countByMajor();
    QVariantList countByGrade();
    QVariantList getAverageGradeByMajor();

    // 验证查询
    bool existsByStudentNumber(const QString& studentNumber);
    bool existsByEmail(const QString& email);
    int getEnrollmentCount(const QString& studentNumber);

    // 业务操作（带事务）
    bool transferStudent(const QString& studentNumber, const QString& newMajor, int newGrade);
    bool graduateStudents(const QList<int>& studentIds);

private:
    // 辅助方法
    QList<Student> convertFromVariantList(const QVariantList& list);
    Student convertFromVariant(const QVariant& variant);
    Student selectOneStudent(const QString& statementId, const QVariantMap& params = {}); // 优雅处理selectOne问题
    QVariantMap createParameterMap(const QString& key, const QVariant& value);
    QVariantMap createParameterMap(const QVariantMap& params);

    // 错误处理
    void logError(const QString& operation, const QString& error);

signals:
    void studentInserted(const Student& student);
    void studentUpdated(const Student& student);
    void studentDeleted(int studentId);
    void errorOccurred(const QString& error);
}; 