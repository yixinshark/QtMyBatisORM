#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QVariantMap>
#include <QDebug>

/**
 * 学生数据模型类
 * 
 * 这个类演示了如何设计一个完整的数据模型类，包含：
 * - 完整的字段定义
 * - 构造函数和析构函数  
 * - 数据转换方法（toMap/fromMap）
 * - 数据验证方法
 * - 调试输出支持
 */
class Student
{
public:
    // 学生状态枚举
    enum class Status {
        ACTIVE,     // 在校
        INACTIVE,   // 休学
        GRADUATED,  // 毕业
        DROPPED,    // 退学
        DELETED     // 已删除
    };

    // 性别枚举
    enum class Gender {
        UNKNOWN,    // 未知
        MALE,       // 男
        FEMALE      // 女
    };

    // 构造函数
    Student();
    Student(const QString& studentNumber, const QString& name, Gender gender = Gender::UNKNOWN);
    Student(const Student& other);
    Student& operator=(const Student& other);
    ~Student() = default;

    // Getter方法
    int id() const { return m_id; }
    QString studentNumber() const { return m_studentNumber; }
    QString name() const { return m_name; }
    Gender gender() const { return m_gender; }
    QDate birthDate() const { return m_birthDate; }
    QString major() const { return m_major; }
    int grade() const { return m_grade; }
    QString phone() const { return m_phone; }
    QString email() const { return m_email; }
    QString address() const { return m_address; }
    QDate enrollmentDate() const { return m_enrollmentDate; }
    Status status() const { return m_status; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }

    // Setter方法
    void setId(int id) { m_id = id; }
    void setStudentNumber(const QString& studentNumber) { m_studentNumber = studentNumber; }
    void setName(const QString& name) { m_name = name; }
    void setGender(Gender gender) { m_gender = gender; }
    void setBirthDate(const QDate& birthDate) { m_birthDate = birthDate; }
    void setMajor(const QString& major) { m_major = major; }
    void setGrade(int grade) { m_grade = grade; }
    void setPhone(const QString& phone) { m_phone = phone; }
    void setEmail(const QString& email) { m_email = email; }
    void setAddress(const QString& address) { m_address = address; }
    void setEnrollmentDate(const QDate& enrollmentDate) { m_enrollmentDate = enrollmentDate; }
    void setStatus(Status status) { m_status = status; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

    // 数据转换方法 - 用于与QtMyBatisORM交互
    QVariantMap toMap() const;
    QVariantMap toUpdateMap() const; // 仅包含update SQL需要的字段
    static Student fromMap(const QVariantMap& map);

    // 便捷方法
    bool isValid() const;
    QString validateData() const;
    QString displayName() const;
    int age() const;
    QString genderString() const;
    QString statusString() const;

    // 工具方法
    bool operator==(const Student& other) const;
    bool operator!=(const Student& other) const;

    // 静态工具方法
    static QString genderToString(Gender gender);
    static Gender stringToGender(const QString& genderStr);
    static QString statusToString(Status status);
    static Status stringToStatus(const QString& statusStr);

private:
    int m_id;                      // 主键ID
    QString m_studentNumber;       // 学号（唯一）
    QString m_name;               // 姓名
    Gender m_gender;              // 性别
    QDate m_birthDate;            // 出生日期
    QString m_major;              // 专业
    int m_grade;                  // 年级
    QString m_phone;              // 电话
    QString m_email;              // 邮箱
    QString m_address;            // 地址
    QDate m_enrollmentDate;       // 入学日期
    Status m_status;              // 状态
    QDateTime m_createdAt;        // 创建时间
    QDateTime m_updatedAt;        // 更新时间
};

// 调试输出支持
QDebug operator<<(QDebug debug, const Student& student);
QDebug operator<<(QDebug debug, Student::Gender gender);
QDebug operator<<(QDebug debug, Student::Status status); 