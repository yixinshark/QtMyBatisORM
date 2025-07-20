#include "course.h"

// 构造函数
Course::Course()
    : m_id(0)
    , m_credits(0)
    , m_hours(0)
    , m_maxStudents(50)
    , m_status(Status::ACTIVE)
{
}

Course::Course(const QString& courseCode, const QString& name)
    : m_id(0)
    , m_courseCode(courseCode)
    , m_name(name)
    , m_credits(0)
    , m_hours(0)
    , m_maxStudents(50)
    , m_status(Status::ACTIVE)
{
}

Course::Course(const Course& other)
    : m_id(other.m_id)
    , m_courseCode(other.m_courseCode)
    , m_name(other.m_name)
    , m_description(other.m_description)
    , m_credits(other.m_credits)
    , m_hours(other.m_hours)
    , m_teacher(other.m_teacher)
    , m_department(other.m_department)
    , m_semester(other.m_semester)
    , m_maxStudents(other.m_maxStudents)
    , m_status(other.m_status)
    , m_createdAt(other.m_createdAt)
    , m_updatedAt(other.m_updatedAt)
{
}

Course& Course::operator=(const Course& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_courseCode = other.m_courseCode;
        m_name = other.m_name;
        m_description = other.m_description;
        m_credits = other.m_credits;
        m_hours = other.m_hours;
        m_teacher = other.m_teacher;
        m_department = other.m_department;
        m_semester = other.m_semester;
        m_maxStudents = other.m_maxStudents;
        m_status = other.m_status;
        m_createdAt = other.m_createdAt;
        m_updatedAt = other.m_updatedAt;
    }
    return *this;
}

// 数据转换方法
QVariantMap Course::toMap() const
{
    QVariantMap map;
    
    if (m_id > 0) {
        map["id"] = m_id;
    }
    
    map["course_code"] = m_courseCode;
    map["name"] = m_name;
    map["description"] = m_description;
    map["credits"] = m_credits;
    map["hours"] = m_hours;
    map["teacher"] = m_teacher;
    map["department"] = m_department;
    map["semester"] = m_semester;
    map["max_students"] = m_maxStudents;
    map["status"] = statusToString(m_status);
    
    if (m_createdAt.isValid()) {
        map["created_at"] = m_createdAt.toString(Qt::ISODate);
    }
    
    if (m_updatedAt.isValid()) {
        map["updated_at"] = m_updatedAt.toString(Qt::ISODate);
    }
    
    return map;
}

Course Course::fromMap(const QVariantMap& map)
{
    Course course;
    
    course.m_id = map["id"].toInt();
    course.m_courseCode = map["course_code"].toString();
    course.m_name = map["name"].toString();
    course.m_description = map["description"].toString();
    course.m_credits = map["credits"].toInt();
    course.m_hours = map["hours"].toInt();
    course.m_teacher = map["teacher"].toString();
    course.m_department = map["department"].toString();
    course.m_semester = map["semester"].toString();
    course.m_maxStudents = map["max_students"].toInt();
    course.m_status = stringToStatus(map["status"].toString());
    
    QString createdAtStr = map["created_at"].toString();
    if (!createdAtStr.isEmpty()) {
        course.m_createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
    }
    
    QString updatedAtStr = map["updated_at"].toString();
    if (!updatedAtStr.isEmpty()) {
        course.m_updatedAt = QDateTime::fromString(updatedAtStr, Qt::ISODate);
    }
    
    return course;
}

// 便捷方法
bool Course::isValid() const
{
    return !m_courseCode.isEmpty() && !m_name.isEmpty();
}

QString Course::validateData() const
{
    QStringList errors;
    
    if (m_courseCode.isEmpty()) {
        errors << "课程代码不能为空";
    } else if (m_courseCode.length() > 20) {
        errors << "课程代码长度不能超过20个字符";
    }
    
    if (m_name.isEmpty()) {
        errors << "课程名称不能为空";
    } else if (m_name.length() > 200) {
        errors << "课程名称长度不能超过200个字符";
    }
    
    if (m_credits < 0 || m_credits > 20) {
        errors << "学分应在0-20之间";
    }
    
    if (m_hours < 0 || m_hours > 500) {
        errors << "学时应在0-500之间";
    }
    
    if (m_maxStudents < 1 || m_maxStudents > 1000) {
        errors << "最大学生数应在1-1000之间";
    }
    
    return errors.join("; ");
}

QString Course::displayName() const
{
    if (!m_courseCode.isEmpty() && !m_name.isEmpty()) {
        return QString("%1 - %2").arg(m_courseCode, m_name);
    } else if (!m_name.isEmpty()) {
        return m_name;
    } else if (!m_courseCode.isEmpty()) {
        return m_courseCode;
    }
    return "未知课程";
}

QString Course::statusString() const
{
    return statusToString(m_status);
}

// 工具方法
bool Course::operator==(const Course& other) const
{
    return m_id == other.m_id && m_courseCode == other.m_courseCode;
}

bool Course::operator!=(const Course& other) const
{
    return !(*this == other);
}

// 静态工具方法
QString Course::statusToString(Status status)
{
    switch (status) {
        case Status::ACTIVE: return "ACTIVE";
        case Status::INACTIVE: return "INACTIVE";
        case Status::COMPLETED: return "COMPLETED";
        case Status::CANCELLED: return "CANCELLED";
        case Status::DELETED: return "DELETED";
        default: return "UNKNOWN";
    }
}

Course::Status Course::stringToStatus(const QString& statusStr)
{
    if (statusStr == "ACTIVE") return Status::ACTIVE;
    if (statusStr == "INACTIVE") return Status::INACTIVE;
    if (statusStr == "COMPLETED") return Status::COMPLETED;
    if (statusStr == "CANCELLED") return Status::CANCELLED;
    if (statusStr == "DELETED") return Status::DELETED;
    return Status::ACTIVE;
}

// 调试输出支持
QDebug operator<<(QDebug debug, const Course& course)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << QString("Course(%1, %2, %3, %4学分)")
                       .arg(course.id())
                       .arg(course.courseCode())
                       .arg(course.name())
                       .arg(course.credits());
    return debug;
}

QDebug operator<<(QDebug debug, Course::Status status)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << Course::statusToString(status);
    return debug;
} 