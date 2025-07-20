#pragma once

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QDebug>

/**
 * 课程数据模型类
 */
class Course
{
public:
    // 课程状态枚举
    enum class Status {
        ACTIVE,     // 开课
        INACTIVE,   // 暂停
        COMPLETED,  // 已结课
        CANCELLED,  // 取消
        DELETED     // 已删除
    };

    // 构造函数
    Course();
    Course(const QString& courseCode, const QString& name);
    Course(const Course& other);
    Course& operator=(const Course& other);
    ~Course() = default;

    // Getter方法
    int id() const { return m_id; }
    QString courseCode() const { return m_courseCode; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    int credits() const { return m_credits; }
    int hours() const { return m_hours; }
    QString teacher() const { return m_teacher; }
    QString department() const { return m_department; }
    QString semester() const { return m_semester; }
    int maxStudents() const { return m_maxStudents; }
    Status status() const { return m_status; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }

    // Setter方法
    void setId(int id) { m_id = id; }
    void setCourseCode(const QString& courseCode) { m_courseCode = courseCode; }
    void setName(const QString& name) { m_name = name; }
    void setDescription(const QString& description) { m_description = description; }
    void setCredits(int credits) { m_credits = credits; }
    void setHours(int hours) { m_hours = hours; }
    void setTeacher(const QString& teacher) { m_teacher = teacher; }
    void setDepartment(const QString& department) { m_department = department; }
    void setSemester(const QString& semester) { m_semester = semester; }
    void setMaxStudents(int maxStudents) { m_maxStudents = maxStudents; }
    void setStatus(Status status) { m_status = status; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

    // 数据转换方法
    QVariantMap toMap() const;
    static Course fromMap(const QVariantMap& map);

    // 便捷方法
    bool isValid() const;
    QString validateData() const;
    QString displayName() const;
    QString statusString() const;

    // 工具方法
    bool operator==(const Course& other) const;
    bool operator!=(const Course& other) const;

    // 静态工具方法
    static QString statusToString(Status status);
    static Status stringToStatus(const QString& statusStr);

private:
    int m_id;                      // 主键ID
    QString m_courseCode;          // 课程代码（唯一）
    QString m_name;               // 课程名称
    QString m_description;        // 课程描述
    int m_credits;                // 学分
    int m_hours;                  // 学时
    QString m_teacher;            // 授课教师
    QString m_department;         // 开课院系
    QString m_semester;           // 学期
    int m_maxStudents;            // 最大学生数
    Status m_status;              // 状态
    QDateTime m_createdAt;        // 创建时间
    QDateTime m_updatedAt;        // 更新时间
};

// 调试输出支持
QDebug operator<<(QDebug debug, const Course& course);
QDebug operator<<(QDebug debug, Course::Status status); 