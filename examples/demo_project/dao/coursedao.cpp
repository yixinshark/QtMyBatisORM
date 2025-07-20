#include "coursedao.h"
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>
#include <QDebug>

using namespace QtMyBatisORM;

CourseDao::CourseDao(QObject* parent)
    : QObject(parent)
{
}

QList<Course> CourseDao::findAll()
{
    try {
        QVariantList results = QtMyBatisHelper::selectList("Course.findAll");
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findAll", e.message());
        return {};
    }
}

Course CourseDao::findById(int id)
{
    try {
        QVariantMap params;
        params["arg1"] = id;
        QVariant result = QtMyBatisHelper::selectOne("Course.findById", params);
        return convertFromVariant(result);
    } catch (const QtMyBatisException& e) {
        logError("findById", e.message());
        return Course();
    }
}

Course CourseDao::findByCourseCode(const QString& courseCode)
{
    QVariantMap params;
    params["arg1"] = courseCode;
    return selectOneCourse("Course.findByCourseCode", params);
}

int CourseDao::count()
{
    try {
        QVariant result = QtMyBatisHelper::selectOne("Course.count");
        return result.toInt();
    } catch (const QtMyBatisException& e) {
        logError("count", e.message());
        return -1;
    }
}

bool CourseDao::insert(const Course& course)
{
    try {
        QString validationError = course.validateData();
        if (!validationError.isEmpty()) {
            logError("insert", "数据验证失败: " + validationError);
            return false;
        }
        
        if (existsByCourseCode(course.courseCode())) {
            logError("insert", "课程代码已存在: " + course.courseCode());
            return false;
        }
        
        int result = QtMyBatisHelper::insert("Course.insert", course.toMap());
        if (result > 0) {
            emit courseInserted(course);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("insert", e.message());
        return false;
    }
}

bool CourseDao::update(const Course& course)
{
    try {
        QString validationError = course.validateData();
        if (!validationError.isEmpty()) {
            logError("update", "数据验证失败: " + validationError);
            return false;
        }
        
        int result = QtMyBatisHelper::update("Course.update", course.toMap());
        if (result > 0) {
            emit courseUpdated(course);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("update", e.message());
        return false;
    }
}

bool CourseDao::deleteById(int id)
{
    try {
        QVariantMap params;
        params["arg1"] = id;
        int result = QtMyBatisHelper::remove("Course.deleteById", params);
        if (result > 0) {
            emit courseDeleted(id);
            return true;
        }
        return false;
    } catch (const QtMyBatisException& e) {
        logError("deleteById", e.message());
        return false;
    }
}

QList<Course> CourseDao::findByDepartment(const QString& department)
{
    try {
        QVariantMap params;
        params["arg1"] = QString("%%%1%%").arg(department);
        QVariantList results = QtMyBatisHelper::selectList("Course.findByDepartment", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByDepartment", e.message());
        return {};
    }
}

QList<Course> CourseDao::findBySemester(const QString& semester)
{
    try {
        QVariantMap params;
        params["arg1"] = semester;
        QVariantList results = QtMyBatisHelper::selectList("Course.findBySemester", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findBySemester", e.message());
        return {};
    }
}

QList<Course> CourseDao::findByTeacher(const QString& teacher)
{
    try {
        QVariantMap params;
        params["arg1"] = QString("%%%1%%").arg(teacher);
        QVariantList results = QtMyBatisHelper::selectList("Course.findByTeacher", params);
        return convertFromVariantList(results);
    } catch (const QtMyBatisException& e) {
        logError("findByTeacher", e.message());
        return {};
    }
}

bool CourseDao::existsByCourseCode(const QString& courseCode)
{
    try {
        // Workaround: 使用 selectList 替代 selectOne 来避免库bug
        QVariantMap params;
        params["arg1"] = courseCode;
        QVariantList results = QtMyBatisHelper::selectList("Course.findByCourseCode", params);
        return !results.isEmpty();
    } catch (const QtMyBatisException& e) {
        logError("existsByCourseCode", e.message());
        return false;
    }
}

bool CourseDao::canEnrollMore(int courseId)
{
    try {
        QVariantMap params;
        params["arg1"] = courseId;
        QVariant result = QtMyBatisHelper::selectOne("Course.canEnrollMore", params);
        return result.toInt() > 0;
    } catch (const QtMyBatisException& e) {
        logError("canEnrollMore", e.message());
        return false;
    }
}

QList<Course> CourseDao::convertFromVariantList(const QVariantList& list)
{
    QList<Course> courses;
    for (const QVariant& item : list) {
        courses.append(Course::fromMap(item.toMap()));
    }
    return courses;
}

Course CourseDao::convertFromVariant(const QVariant& variant)
{
    if (variant.isNull() || !variant.isValid()) {
        return Course();
    }
    return Course::fromMap(variant.toMap());
}

Course CourseDao::selectOneCourse(const QString& statementId, const QVariantMap& params)
{
    try {
        // 使用selectList然后取第一个结果的方法来处理selectOne的已知问题
        QVariantList results = QtMyBatisHelper::selectList(statementId, params);
        if (!results.isEmpty()) {
            return convertFromVariant(results.first());
        }
        return Course();
    } catch (const QtMyBatisException& e) {
        logError("selectOneCourse", e.message());
        return Course();
    }
}

void CourseDao::logError(const QString& operation, const QString& error)
{
    QString errorMsg = QString("CourseDao::%1 失败: %2").arg(operation, error);
    qWarning() << errorMsg;
    emit errorOccurred(errorMsg);
} 