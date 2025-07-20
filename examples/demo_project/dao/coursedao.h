#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantMap>
#include "../models/course.h"

/**
 * 课程数据访问对象类（简化版）
 */
class CourseDao : public QObject
{
    Q_OBJECT

public:
    explicit CourseDao(QObject* parent = nullptr);
    ~CourseDao() = default;

    // 基础CRUD操作
    QList<Course> findAll();
    Course findById(int id);
    Course findByCourseCode(const QString& courseCode);
    int count();
    bool insert(const Course& course);
    bool update(const Course& course);
    bool deleteById(int id);

    // 条件查询
    QList<Course> findByDepartment(const QString& department);
    QList<Course> findBySemester(const QString& semester);
    QList<Course> findByTeacher(const QString& teacher);

    // 验证查询
    bool existsByCourseCode(const QString& courseCode);
    bool canEnrollMore(int courseId);

private:
    QList<Course> convertFromVariantList(const QVariantList& list);
    Course convertFromVariant(const QVariant& variant);
    Course selectOneCourse(const QString& statementId, const QVariantMap& params = {}); // 优雅处理selectOne问题
    void logError(const QString& operation, const QString& error);

signals:
    void courseInserted(const Course& course);
    void courseUpdated(const Course& course);
    void courseDeleted(int courseId);
    void errorOccurred(const QString& error);
}; 