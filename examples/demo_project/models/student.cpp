#include "student.h"
#include <QRegularExpression>

// 构造函数
Student::Student()
    : m_id(0)
    , m_gender(Gender::UNKNOWN)
    , m_grade(0)
    , m_status(Status::ACTIVE)
{
}

Student::Student(const QString& studentNumber, const QString& name, Gender gender)
    : m_id(0)
    , m_studentNumber(studentNumber)
    , m_name(name)
    , m_gender(gender)
    , m_grade(0)
    , m_status(Status::ACTIVE)
{
}

Student::Student(const Student& other)
    : m_id(other.m_id)
    , m_studentNumber(other.m_studentNumber)
    , m_name(other.m_name)
    , m_gender(other.m_gender)
    , m_birthDate(other.m_birthDate)
    , m_major(other.m_major)
    , m_grade(other.m_grade)
    , m_phone(other.m_phone)
    , m_email(other.m_email)
    , m_address(other.m_address)
    , m_enrollmentDate(other.m_enrollmentDate)
    , m_status(other.m_status)
    , m_createdAt(other.m_createdAt)
    , m_updatedAt(other.m_updatedAt)
{
}

Student& Student::operator=(const Student& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_studentNumber = other.m_studentNumber;
        m_name = other.m_name;
        m_gender = other.m_gender;
        m_birthDate = other.m_birthDate;
        m_major = other.m_major;
        m_grade = other.m_grade;
        m_phone = other.m_phone;
        m_email = other.m_email;
        m_address = other.m_address;
        m_enrollmentDate = other.m_enrollmentDate;
        m_status = other.m_status;
        m_createdAt = other.m_createdAt;
        m_updatedAt = other.m_updatedAt;
    }
    return *this;
}

// 数据转换方法
QVariantMap Student::toMap() const
{
    QVariantMap map;
    
    // 只有ID大于0时才包含ID（用于更新操作）
    if (m_id > 0) {
        map["id"] = m_id;
    }
    
    map["student_number"] = m_studentNumber;
    map["name"] = m_name;
    map["gender"] = genderToString(m_gender);
    
    if (m_birthDate.isValid()) {
        map["birth_date"] = m_birthDate.toString(Qt::ISODate);
    }
    
    map["major"] = m_major;
    map["grade"] = m_grade;
    map["phone"] = m_phone;
    map["email"] = m_email;
    map["address"] = m_address;
    
    if (m_enrollmentDate.isValid()) {
        map["enrollment_date"] = m_enrollmentDate.toString(Qt::ISODate);
    }
    
    map["status"] = statusToString(m_status);
    
    if (m_createdAt.isValid()) {
        map["created_at"] = m_createdAt.toString(Qt::ISODate);
    }
    
    if (m_updatedAt.isValid()) {
        map["updated_at"] = m_updatedAt.toString(Qt::ISODate);
    }
    
    return map;
}

QVariantMap Student::toUpdateMap() const
{
    QVariantMap map;
    
    // 只包含update SQL需要的字段
    map["id"] = m_id;
    map["name"] = m_name;
    map["gender"] = genderToString(m_gender);
    
    if (m_birthDate.isValid()) {
        map["birth_date"] = m_birthDate.toString(Qt::ISODate);
    }
    
    map["major"] = m_major;
    map["grade"] = m_grade;
    map["phone"] = m_phone;
    map["email"] = m_email;
    map["address"] = m_address;
    
    return map;
}

Student Student::fromMap(const QVariantMap& map)
{
    Student student;
    
    student.m_id = map["id"].toInt();
    student.m_studentNumber = map["student_number"].toString();
    student.m_name = map["name"].toString();
    student.m_gender = stringToGender(map["gender"].toString());
    
    QString birthDateStr = map["birth_date"].toString();
    if (!birthDateStr.isEmpty()) {
        student.m_birthDate = QDate::fromString(birthDateStr, Qt::ISODate);
    }
    
    student.m_major = map["major"].toString();
    student.m_grade = map["grade"].toInt();
    student.m_phone = map["phone"].toString();
    student.m_email = map["email"].toString();
    student.m_address = map["address"].toString();
    
    QString enrollmentDateStr = map["enrollment_date"].toString();
    if (!enrollmentDateStr.isEmpty()) {
        student.m_enrollmentDate = QDate::fromString(enrollmentDateStr, Qt::ISODate);
    }
    
    student.m_status = stringToStatus(map["status"].toString());
    
    QString createdAtStr = map["created_at"].toString();
    if (!createdAtStr.isEmpty()) {
        student.m_createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
    }
    
    QString updatedAtStr = map["updated_at"].toString();
    if (!updatedAtStr.isEmpty()) {
        student.m_updatedAt = QDateTime::fromString(updatedAtStr, Qt::ISODate);
    }
    
    return student;
}

// 便捷方法
bool Student::isValid() const
{
    return !m_studentNumber.isEmpty() && !m_name.isEmpty();
}

QString Student::validateData() const
{
    QStringList errors;
    
    if (m_studentNumber.isEmpty()) {
        errors << "学号不能为空";
    } else if (m_studentNumber.length() < 6 || m_studentNumber.length() > 20) {
        errors << "学号长度应在6-20个字符之间";
    }
    
    if (m_name.isEmpty()) {
        errors << "姓名不能为空";
    } else if (m_name.length() > 100) {
        errors << "姓名长度不能超过100个字符";
    }
    
    if (!m_email.isEmpty()) {
        QRegularExpression emailRegex("^[\\w\\.-]+@[\\w\\.-]+\\.[a-zA-Z]{2,}$");
        if (!emailRegex.match(m_email).hasMatch()) {
            errors << "邮箱格式不正确";
        }
    }
    
    if (!m_phone.isEmpty()) {
        QRegularExpression phoneRegex("^[1][3-9]\\d{9}$");
        if (!phoneRegex.match(m_phone).hasMatch()) {
            errors << "手机号格式不正确";
        }
    }
    
    if (m_grade < 0 || m_grade > 9999) {
        errors << "年级应在0-9999之间";
    }
    
    if (m_birthDate.isValid() && m_birthDate > QDate::currentDate()) {
        errors << "出生日期不能大于当前日期";
    }
    
    return errors.join("; ");
}

QString Student::displayName() const
{
    if (!m_studentNumber.isEmpty() && !m_name.isEmpty()) {
        return QString("%1 (%2)").arg(m_name, m_studentNumber);
    } else if (!m_name.isEmpty()) {
        return m_name;
    } else if (!m_studentNumber.isEmpty()) {
        return m_studentNumber;
    }
    return "未知学生";
}

int Student::age() const
{
    if (!m_birthDate.isValid()) {
        return -1;
    }
    
    QDate today = QDate::currentDate();
    int years = today.year() - m_birthDate.year();
    
    if (today.month() < m_birthDate.month() ||
        (today.month() == m_birthDate.month() && today.day() < m_birthDate.day())) {
        years--;
    }
    
    return years;
}

QString Student::genderString() const
{
    return genderToString(m_gender);
}

QString Student::statusString() const
{
    return statusToString(m_status);
}

// 工具方法
bool Student::operator==(const Student& other) const
{
    return m_id == other.m_id && m_studentNumber == other.m_studentNumber;
}

bool Student::operator!=(const Student& other) const
{
    return !(*this == other);
}

// 静态工具方法
QString Student::genderToString(Gender gender)
{
    switch (gender) {
        case Gender::MALE: return "男";
        case Gender::FEMALE: return "女";
        case Gender::UNKNOWN: 
        default: return "未知";
    }
}

Student::Gender Student::stringToGender(const QString& genderStr)
{
    if (genderStr == "男") return Gender::MALE;
    if (genderStr == "女") return Gender::FEMALE;
    return Gender::UNKNOWN;
}

QString Student::statusToString(Status status)
{
    switch (status) {
        case Status::ACTIVE: return "ACTIVE";
        case Status::INACTIVE: return "INACTIVE";
        case Status::GRADUATED: return "GRADUATED";
        case Status::DROPPED: return "DROPPED";
        case Status::DELETED: return "DELETED";
        default: return "UNKNOWN";
    }
}

Student::Status Student::stringToStatus(const QString& statusStr)
{
    if (statusStr == "ACTIVE") return Status::ACTIVE;
    if (statusStr == "INACTIVE") return Status::INACTIVE;
    if (statusStr == "GRADUATED") return Status::GRADUATED;
    if (statusStr == "DROPPED") return Status::DROPPED;
    if (statusStr == "DELETED") return Status::DELETED;
    return Status::ACTIVE;
}

// 调试输出支持
QDebug operator<<(QDebug debug, const Student& student)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << QString("Student(%1, %2, %3, %4)")
                       .arg(student.id())
                       .arg(student.studentNumber())
                       .arg(student.name())
                       .arg(student.genderString());
    return debug;
}

QDebug operator<<(QDebug debug, Student::Gender gender)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << Student::genderToString(gender);
    return debug;
}

QDebug operator<<(QDebug debug, Student::Status status)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << Student::statusToString(status);
    return debug;
} 