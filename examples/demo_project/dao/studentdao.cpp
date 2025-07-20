#include "studentdao.h"
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>
#include <QDebug>
#include <QDate>

using namespace QtMyBatisORM;

StudentDao::StudentDao(QObject* parent)
    : QObject(parent)
{
}

// 基础CRUD操作
QList<Student> StudentDao::findAll()
{
    try {
        QVariantList results = QtMyBatisHelper::selectList("Student.findAll");
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findAll", e.message());
        return {};
    }
}

Student StudentDao::findById(int id)
{
    try {
        QVariantMap params;
        params["arg1"] = id;
        QVariant result = QtMyBatisHelper::selectOne("Student.findById", params);
        return convertFromVariant(result);
    } catch (const QtMyBatisException& e) {
        logError("findById", e.message());
        return Student();
    }
}

Student StudentDao::findByStudentNumber(const QString& studentNumber)
{
    QVariantMap params;
    params["arg1"] = studentNumber;
    return selectOneStudent("Student.findByStudentNumber", params);
}

int StudentDao::count()
{
    try {
        QVariant result = QtMyBatisHelper::selectOne("Student.count");
        return result.toInt();
    } catch (const QtMyBatisException& e) {
        logError("count", e.message());
        return -1;
    }
}

bool StudentDao::insert(const Student& student)
{
    try {
        // 数据验证
        QString validationError = student.validateData();
        if (!validationError.isEmpty()) {
            logError("insert", "数据验证失败: " + validationError);
            return false;
        }
        
        // 检查学号是否已存在
        if (existsByStudentNumber(student.studentNumber())) {
            logError("insert", "学号已存在: " + student.studentNumber());
            return false;
        }
        
        int result = QtMyBatisHelper::insert("Student.insert", student.toMap());
        if (result > 0) {
            emit studentInserted(student);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("insert", e.message());
        return false;
    }
}

bool StudentDao::update(const Student& student)
{
    try {
        // 数据验证
        QString validationError = student.validateData();
        if (!validationError.isEmpty()) {
            logError("update", "数据验证失败: " + validationError);
            return false;
        }
        
        int result = QtMyBatisHelper::update("Student.update", student.toUpdateMap());
        if (result > 0) {
            emit studentUpdated(student);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("update", e.message());
        return false;
    }
}

bool StudentDao::deleteById(int id)
{
    try {
        QVariantMap params;
        params["arg1"] = id;
        int result = QtMyBatisHelper::remove("Student.deleteById", params);
        if (result > 0) {
            emit studentDeleted(id);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("deleteById", e.message());
        return false;
    }
}

// 条件查询
QList<Student> StudentDao::findByMajor(const QString& major)
{
    try {
        QVariantMap params;
        params["arg1"] = QString("%%%1%%").arg(major);
        QVariantList results = QtMyBatisHelper::selectList("Student.findByMajor", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByMajor", e.message());
        return {};
    }
}

QList<Student> StudentDao::findByGrade(int grade)
{
    try {
        QVariantMap params;
        params["arg1"] = grade;
        QVariantList results = QtMyBatisHelper::selectList("Student.findByGrade", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByGrade", e.message());
        return {};
    }
}

QList<Student> StudentDao::findByNamePattern(const QString& namePattern)
{
    try {
        QVariantMap params;
        params["arg1"] = QString("%%%1%%").arg(namePattern);
        QVariantList results = QtMyBatisHelper::selectList("Student.findByNamePattern", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByNamePattern", e.message());
        return {};
    }
}

// 分页查询
QList<Student> StudentDao::findWithPagination(int limit, int offset)
{
    try {
        QVariantMap params;
        params["arg1"] = limit;
        params["arg2"] = offset;
        QVariantList results = QtMyBatisHelper::selectList("Student.findWithPagination", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findWithPagination", e.message());
        return {};
    }
}

QList<Student> StudentDao::findByMajorWithPagination(const QString& major, int limit, int offset)
{
    try {
        QVariantMap params;
        params["arg1"] = QString("%%%1%%").arg(major);
        params["arg2"] = limit;
        params["arg3"] = offset;
        QVariantList results = QtMyBatisHelper::selectList("Student.findByMajorWithPagination", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByMajorWithPagination", e.message());
        return {};
    }
}

// 高级查询
QList<Student> StudentDao::findStudentsWithCourseCount()
{
    try {
        QVariantList results = QtMyBatisHelper::selectList("Student.findStudentsWithCourseCount");
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findStudentsWithCourseCount", e.message());
        return {};
    }
}

QList<Student> StudentDao::findStudentsInCourse(int courseId)
{
    try {
        QVariantMap params;
        params["arg1"] = courseId;
        QVariantList results = QtMyBatisHelper::selectList("Student.findStudentsInCourse", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findStudentsInCourse", e.message());
        return {};
    }
}

QList<Student> StudentDao::findTopStudentsByGPA(double minGPA, int limit)
{
    try {
        QVariantMap params;
        params["arg1"] = minGPA;
        params["arg2"] = limit;
        QVariantList results = QtMyBatisHelper::selectList("Student.findTopStudentsByGPA", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findTopStudentsByGPA", e.message());
        return {};
    }
}

// 批量操作
bool StudentDao::batchInsert(const QList<Student>& students)
{
    try {
        QList<QVariantMap> paramsList;
        for (const Student& student : students) {
            // 数据验证
            QString validationError = student.validateData();
            if (!validationError.isEmpty()) {
                logError("batchInsert", QString("学生 %1 数据验证失败: %2")
                        .arg(student.studentNumber(), validationError));
                return false;
            }
            paramsList.append(student.toMap());
        }
        
        int result = QtMyBatisHelper::batchInsert("Student.insert", paramsList);
        return result == students.size();
    } catch (const QtMyBatisException& e) {
        logError("batchInsert", e.message());
        return false;
    }
}

bool StudentDao::batchUpdateGrade(const QStringList& studentNumbers, int grade)
{
    try {
        QVariantMap params;
        params["grade"] = grade;
        params["student_numbers"] = studentNumbers.join(",");
        
        int result = QtMyBatisHelper::update("Student.batchUpdateGrade", params);
        return result > 0;
    } catch (const QtMyBatisException& e) {
        logError("batchUpdateGrade", e.message());
        return false;
    }
}

bool StudentDao::batchUpdateMajor(const QList<int>& studentIds, const QString& major)
{
    try {
        QStringList idStrings;
        for (int id : studentIds) {
            idStrings << QString::number(id);
        }
        
        QVariantMap params;
        params["major"] = major;
        params["student_ids"] = idStrings.join(",");
        
        int result = QtMyBatisHelper::update("Student.batchUpdateMajor", params);
        return result > 0;
    } catch (const QtMyBatisException& e) {
        logError("batchUpdateMajor", e.message());
        return false;
    }
}

// 统计查询
QVariantList StudentDao::countByMajor()
{
    try {
        return QtMyBatisHelper::selectList("Student.countByMajor");
    } catch (const QtMyBatisException& e) {
        logError("countByMajor", e.message());
        return {};
    }
}

QVariantList StudentDao::countByGrade()
{
    try {
        return QtMyBatisHelper::selectList("Student.countByGrade");
    } catch (const QtMyBatisException& e) {
        logError("countByGrade", e.message());
        return {};
    }
}

QVariantList StudentDao::getAverageGradeByMajor()
{
    try {
        return QtMyBatisHelper::selectList("Student.getAverageGradeByMajor");
    } catch (const QtMyBatisException& e) {
        logError("getAverageGradeByMajor", e.message());
        return {};
    }
}

// 验证查询
bool StudentDao::existsByStudentNumber(const QString& studentNumber)
{
    try {
        // Workaround: 使用 selectList 替代 selectOne 来避免库bug
        QVariantMap params;
        params["arg1"] = studentNumber;
        QVariantList results = QtMyBatisHelper::selectList("Student.findByStudentNumber", params);
        return !results.isEmpty();
    } catch (const QtMyBatisException& e) {
        logError("existsByStudentNumber", e.message());
        return false;
    }
}

bool StudentDao::existsByEmail(const QString& email)
{
    try {
        QVariantMap params;
        params["arg1"] = email;
        QVariant result = QtMyBatisHelper::selectOne("Student.existsByEmail", params);
        return result.toInt() > 0;
    } catch (const QtMyBatisException& e) {
        logError("existsByEmail", e.message());
        return false;
    }
}

int StudentDao::getEnrollmentCount(const QString& studentNumber)
{
    try {
        QVariantMap params;
        params["arg1"] = studentNumber;
        QVariant result = QtMyBatisHelper::selectOne("Student.checkEnrollmentEligibility", params);
        return result.toInt();
    } catch (const QtMyBatisException& e) {
        logError("getEnrollmentCount", e.message());
        return -1;
    }
}

// 业务操作（带事务）
bool StudentDao::transferStudent(const QString& studentNumber, const QString& newMajor, int newGrade)
{
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            // 查找学生
            Student student = findByStudentNumber(studentNumber);
            if (!student.isValid()) {
                logError("transferStudent", "学生不存在: " + studentNumber);
                return false;
            }
            
            // 更新学生信息
            student.setMajor(newMajor);
            student.setGrade(newGrade);
            
            // 保存更改
            return update(student);
        } catch (const QtMyBatisException& e) {
            logError("transferStudent", e.message());
            return false;
        }
    });
}

bool StudentDao::graduateStudents(const QList<int>& studentIds)
{
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            for (int studentId : studentIds) {
                QVariantMap params;
                params["id"] = studentId;
                params["status"] = Student::statusToString(Student::Status::GRADUATED);
                
                int result = QtMyBatisHelper::update("Student.updateStatus", params);
                if (result <= 0) {
                    logError("graduateStudents", QString("更新学生状态失败: %1").arg(studentId));
                    return false;
                }
            }
            return true;
        } catch (const QtMyBatisException& e) {
            logError("graduateStudents", e.message());
            return false;
        }
    });
}

// 辅助方法
QList<Student> StudentDao::convertFromVariantList(const QVariantList& list)
{
    QList<Student> students;
    for (const QVariant& item : list) {
        students.append(Student::fromMap(item.toMap()));
    }
    return students;
}

Student StudentDao::convertFromVariant(const QVariant& variant)
{
    if (variant.isNull() || !variant.isValid()) {
        return Student();
    }
    return Student::fromMap(variant.toMap());
}

QVariantMap StudentDao::createParameterMap(const QString& key, const QVariant& value)
{
    QVariantMap params;
    params[key] = value;
    return params;
}

QVariantMap StudentDao::createParameterMap(const QVariantMap& params)
{
    return params;
}

Student StudentDao::selectOneStudent(const QString& statementId, const QVariantMap& params)
{
    try {
        // 使用selectList然后取第一个结果的方法来处理selectOne的已知问题
        QVariantList results = QtMyBatisHelper::selectList(statementId, params);
        if (!results.isEmpty()) {
            return convertFromVariant(results.first());
        }
        return Student();
    } catch (const QtMyBatisException& e) {
        logError("selectOneStudent", e.message());
        return Student();
    }
}

void StudentDao::logError(const QString& operation, const QString& error)
{
    QString errorMsg = QString("StudentDao::%1 失败: %2").arg(operation, error);
    qWarning() << errorMsg;
    emit errorOccurred(errorMsg);
} 