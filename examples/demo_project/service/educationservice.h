#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include "../models/student.h"
#include "../models/course.h"

class StudentDao;
class CourseDao;

/**
 * 教育管理服务类
 * 
 * 演示业务逻辑层的设计，包含：
 * - 复杂业务操作
 * - 跨表事务处理
 * - 数据校验和业务规则
 * - 统计分析功能
 */
class EducationService : public QObject
{
    Q_OBJECT

public:
    struct EnrollmentInfo {
        int enrollmentId;
        Student student;
        Course course;
        QString enrollmentDate;
        double grade;
        QString status;
    };

    struct Statistics {
        int totalStudents;
        int totalCourses;
        int totalEnrollments;
        double averageGPA;
        QString mostPopularMajor;
        QString mostPopularCourse;
    };

    explicit EducationService(QObject* parent = nullptr);
    ~EducationService();

    // 初始化
    bool initialize();
    void shutdown();

    // 学生管理
    QList<Student> getAllStudents();
    Student getStudentById(int id);
    Student getStudentByNumber(const QString& studentNumber);
    bool addStudent(const Student& student);
    bool updateStudent(const Student& student);
    bool removeStudent(int id);
    QList<Student> searchStudents(const QString& keyword);

    // 课程管理
    QList<Course> getAllCourses();
    Course getCourseById(int id);
    Course getCourseByCode(const QString& courseCode);
    bool addCourse(const Course& course);
    bool updateCourse(const Course& course);
    bool removeCourse(int id);
    QList<Course> searchCourses(const QString& keyword);

    // 选课管理
    bool enrollStudent(const QString& studentNumber, const QString& courseCode);
    bool dropCourse(const QString& studentNumber, const QString& courseCode);
    bool updateGrade(const QString& studentNumber, const QString& courseCode, double grade);
    QList<EnrollmentInfo> getStudentEnrollments(const QString& studentNumber);
    QList<EnrollmentInfo> getCourseEnrollments(const QString& courseCode);

    // 业务操作
    bool transferStudent(const QString& studentNumber, const QString& newMajor, int newGrade);
    bool graduateStudents(const QStringList& studentNumbers);
    bool createCourseSchedule(const QString& semester, const QList<Course>& courses);

    // 统计分析
    Statistics getOverallStatistics();
    QVariantList getEnrollmentStatistics();
    QVariantList getGradeDistribution();
    QList<Student> getTopStudents(int limit = 10);
    QList<Course> getPopularCourses(int limit = 10);

    // 数据验证
    QString validateStudent(const Student& student);
    QString validateCourse(const Course& course);
    QString validateEnrollment(const QString& studentNumber, const QString& courseCode);

private:
    StudentDao* m_studentDao;
    CourseDao* m_courseDao;
    bool m_initialized;

    // 私有辅助方法
    bool initializeDatabase();
    void createSampleData();
    bool isStudentEnrolledInCourse(const QString& studentNumber, const QString& courseCode);
    int getStudentIdByNumber(const QString& studentNumber);
    int getCourseIdByCode(const QString& courseCode);

signals:
    void studentEnrolled(const QString& studentNumber, const QString& courseCode);
    void courseDropped(const QString& studentNumber, const QString& courseCode);
    void gradeUpdated(const QString& studentNumber, const QString& courseCode, double grade);
    void statisticsUpdated();
    void errorOccurred(const QString& error);
}; 