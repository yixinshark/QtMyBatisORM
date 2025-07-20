#include "educationservice.h"
#include "../dao/studentdao.h"
#include "../dao/coursedao.h"
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>
#include <QDebug>
#include <QDate>

using namespace QtMyBatisORM;

EducationService::EducationService(QObject* parent)
    : QObject(parent)
    , m_studentDao(nullptr)
    , m_courseDao(nullptr)
    , m_initialized(false)
{
}

EducationService::~EducationService()
{
    shutdown();
}

bool EducationService::initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        // 初始化QtMyBatisORM
        if (!QtMyBatisHelper::initialize(":/resources/config/database.json")) {
            qCritical() << "Failed to initialize QtMyBatisORM";
            return false;
        }

        // 开启调试模式
        QtMyBatisHelper::enableDebugMode(true);

        // 创建DAO对象
        m_studentDao = new StudentDao(this);
        m_courseDao = new CourseDao(this);

        // 初始化数据库
        if (!initializeDatabase()) {
            qCritical() << "Failed to initialize database";
            return false;
        }

        // 创建示例数据
        createSampleData();

        m_initialized = true;
        qDebug() << "EducationService initialized successfully";
        return true;

    } catch (const QtMyBatisException& e) {
        qCritical() << "Failed to initialize EducationService:" << e.message();
        return false;
    }
}

void EducationService::shutdown()
{
    if (m_initialized) {
        delete m_studentDao;
        delete m_courseDao;
        m_studentDao = nullptr;
        m_courseDao = nullptr;

        QtMyBatisHelper::shutdown();
        m_initialized = false;
        qDebug() << "EducationService shutdown";
    }
}

// 学生管理
QList<Student> EducationService::getAllStudents()
{
    if (!m_initialized) return {};
    return m_studentDao->findAll();
}

Student EducationService::getStudentById(int id)
{
    if (!m_initialized) return Student();
    return m_studentDao->findById(id);
}

Student EducationService::getStudentByNumber(const QString& studentNumber)
{
    if (!m_initialized) return Student();
    return m_studentDao->findByStudentNumber(studentNumber);
}

bool EducationService::addStudent(const Student& student)
{
    if (!m_initialized) return false;
    
    QString error = validateStudent(student);
    if (!error.isEmpty()) {
        emit errorOccurred("学生数据验证失败: " + error);
        return false;
    }
    
    return m_studentDao->insert(student);
}

bool EducationService::updateStudent(const Student& student)
{
    if (!m_initialized) return false;
    
    QString error = validateStudent(student);
    if (!error.isEmpty()) {
        emit errorOccurred("学生数据验证失败: " + error);
        return false;
    }
    
    return m_studentDao->update(student);
}

bool EducationService::removeStudent(int id)
{
    if (!m_initialized) return false;
    return m_studentDao->deleteById(id);
}

QList<Student> EducationService::searchStudents(const QString& keyword)
{
    if (!m_initialized) return {};
    
    QList<Student> results;
    
    // 按姓名搜索
    QList<Student> nameResults = m_studentDao->findByNamePattern(keyword);
    results.append(nameResults);
    
    // 按专业搜索
    QList<Student> majorResults = m_studentDao->findByMajor(keyword);
    for (const Student& student : majorResults) {
        bool found = false;
        for (const Student& existing : results) {
            if (existing.id() == student.id()) {
                found = true;
                break;
            }
        }
        if (!found) {
            results.append(student);
        }
    }
    
    return results;
}

// 课程管理
QList<Course> EducationService::getAllCourses()
{
    if (!m_initialized) return {};
    return m_courseDao->findAll();
}

Course EducationService::getCourseById(int id)
{
    if (!m_initialized) return Course();
    return m_courseDao->findById(id);
}

Course EducationService::getCourseByCode(const QString& courseCode)
{
    if (!m_initialized) return Course();
    return m_courseDao->findByCourseCode(courseCode);
}

bool EducationService::addCourse(const Course& course)
{
    if (!m_initialized) return false;
    
    QString error = validateCourse(course);
    if (!error.isEmpty()) {
        emit errorOccurred("课程数据验证失败: " + error);
        return false;
    }
    
    return m_courseDao->insert(course);
}

bool EducationService::updateCourse(const Course& course)
{
    if (!m_initialized) return false;
    
    QString error = validateCourse(course);
    if (!error.isEmpty()) {
        emit errorOccurred("课程数据验证失败: " + error);
        return false;
    }
    
    return m_courseDao->update(course);
}

bool EducationService::removeCourse(int id)
{
    if (!m_initialized) return false;
    return m_courseDao->deleteById(id);
}

QList<Course> EducationService::searchCourses(const QString& keyword)
{
    if (!m_initialized) return {};
    
    QList<Course> results;
    
    // 按院系搜索
    QList<Course> deptResults = m_courseDao->findByDepartment(keyword);
    results.append(deptResults);
    
    // 按教师搜索
    QList<Course> teacherResults = m_courseDao->findByTeacher(keyword);
    for (const Course& course : teacherResults) {
        bool found = false;
        for (const Course& existing : results) {
            if (existing.id() == course.id()) {
                found = true;
                break;
            }
        }
        if (!found) {
            results.append(course);
        }
    }
    
    return results;
}

// 选课管理
bool EducationService::enrollStudent(const QString& studentNumber, const QString& courseCode)
{
    if (!m_initialized) return false;
    
    QString error = validateEnrollment(studentNumber, courseCode);
    if (!error.isEmpty()) {
        emit errorOccurred("选课验证失败: " + error);
        return false;
    }
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            int studentId = getStudentIdByNumber(studentNumber);
            int courseId = getCourseIdByCode(courseCode);
            
            if (studentId <= 0 || courseId <= 0) {
                return false;
            }
            
            QVariantMap params;
            params["student_id"] = studentId;
            params["course_id"] = courseId;
            params["enrollment_date"] = QDate::currentDate().toString(Qt::ISODate);
            params["status"] = "ENROLLED";
            
            int result = QtMyBatisHelper::insert("Enrollment.insert", params);
            if (result > 0) {
                emit studentEnrolled(studentNumber, courseCode);
                return true;
            }
            return false;
        } catch (const QtMyBatisException& e) {
            qWarning() << "选课失败:" << e.message();
            return false;
        }
    });
}

bool EducationService::dropCourse(const QString& studentNumber, const QString& courseCode)
{
    if (!m_initialized) return false;
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            int studentId = getStudentIdByNumber(studentNumber);
            int courseId = getCourseIdByCode(courseCode);
            
            if (studentId <= 0 || courseId <= 0) {
                return false;
            }
            
            QVariantMap params;
            params["arg1"] = studentId;
            params["arg2"] = courseId;
            
            int result = QtMyBatisHelper::remove("Enrollment.deleteByStudentAndCourse", params);
            if (result > 0) {
                emit courseDropped(studentNumber, courseCode);
                return true;
            }
            return false;
        } catch (const QtMyBatisException& e) {
            qWarning() << "退课失败:" << e.message();
            return false;
        }
    });
}

bool EducationService::updateGrade(const QString& studentNumber, const QString& courseCode, double grade)
{
    if (!m_initialized) return false;
    
    if (grade < 0 || grade > 100) {
        emit errorOccurred("成绩必须在0-100之间");
        return false;
    }
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            int studentId = getStudentIdByNumber(studentNumber);
            int courseId = getCourseIdByCode(courseCode);
            
            if (studentId <= 0 || courseId <= 0) {
                return false;
            }
            
            // 查找选课记录
            QVariantMap findParams;
            findParams["arg1"] = studentId;
            findParams["arg2"] = courseId;
            
            QVariant enrollment = QtMyBatisHelper::selectOne("Enrollment.existsEnrollment", findParams);
            if (enrollment.toInt() <= 0) {
                emit errorOccurred("选课记录不存在");
                return false;
            }
            
            // 更新成绩
            QVariantMap updateParams;
            updateParams["grade"] = grade;
            updateParams["student_id"] = studentId;
            updateParams["course_id"] = courseId;
            updateParams["status"] = "COMPLETED"; // 设置为已完成状态
            
            int result = QtMyBatisHelper::update("Enrollment.updateGrade", updateParams);
            if (result > 0) {
                emit gradeUpdated(studentNumber, courseCode, grade);
                return true;
            }
            return false;
        } catch (const QtMyBatisException& e) {
            qWarning() << "更新成绩失败:" << e.message();
            return false;
        }
    });
}

// 统计分析
EducationService::Statistics EducationService::getOverallStatistics()
{
    Statistics stats = {};
    
    if (!m_initialized) return stats;
    
    try {
        stats.totalStudents = m_studentDao->count();
        stats.totalCourses = m_courseDao->count();
        
        QVariant enrollmentCount = QtMyBatisHelper::selectOne("Enrollment.count");
        stats.totalEnrollments = enrollmentCount.toInt();
        
        // 获取平均GPA
        QVariantList avgGrades = QtMyBatisHelper::selectList("Enrollment.getAverageGradeByStudent");
        if (!avgGrades.isEmpty()) {
            double totalGPA = 0;
            int validCount = 0;
            for (const QVariant& item : avgGrades) {
                QVariantMap map = item.toMap();
                double gpa = map["average_grade"].toDouble();
                if (gpa > 0) {
                    totalGPA += gpa;
                    validCount++;
                }
            }
            if (validCount > 0) {
                stats.averageGPA = totalGPA / validCount;
            }
        }
        
        // 获取最受欢迎的专业
        QVariantList majorStats = m_studentDao->countByMajor();
        if (!majorStats.isEmpty()) {
            QVariantMap topMajor = majorStats.first().toMap();
            stats.mostPopularMajor = topMajor["major"].toString();
        }
        
        // 获取最受欢迎的课程
        QVariantMap courseParams;
        courseParams["arg1"] = 1; // 只获取最受欢迎的一门课程
        QVariantList courseStats = QtMyBatisHelper::selectList("Enrollment.getMostPopularCourses", courseParams);
        if (!courseStats.isEmpty()) {
            QVariantMap topCourse = courseStats.first().toMap();
            stats.mostPopularCourse = topCourse["name"].toString();
        }
        
    } catch (const QtMyBatisException& e) {
        qWarning() << "获取统计信息失败:" << e.message();
    }
    
    return stats;
}

// 数据验证
QString EducationService::validateStudent(const Student& student)
{
    return student.validateData();
}

QString EducationService::validateCourse(const Course& course)
{
    return course.validateData();
}

QString EducationService::validateEnrollment(const QString& studentNumber, const QString& courseCode)
{
    QStringList errors;
    
    // 检查学生是否存在
    Student student = getStudentByNumber(studentNumber);
    if (!student.isValid()) {
        errors << "学生不存在";
    }
    
    // 检查课程是否存在
    Course course = getCourseByCode(courseCode);
    if (!course.isValid()) {
        errors << "课程不存在";
    }
    
    // 检查是否已经选过该课程
    if (isStudentEnrolledInCourse(studentNumber, courseCode)) {
        errors << "学生已选择该课程";
    }
    
    // 检查课程是否还有空位
    if (!m_courseDao->canEnrollMore(course.id())) {
        errors << "课程已满";
    }
    
    return errors.join("; ");
}

// 私有辅助方法
bool EducationService::initializeDatabase()
{
    try {
        // 创建表
        QtMyBatisHelper::update("Init.createStudentTable");
        QtMyBatisHelper::update("Init.createCourseTable");
        QtMyBatisHelper::update("Init.createEnrollmentTable");
        QtMyBatisHelper::update("Init.createIndexStudentsNumber");
        QtMyBatisHelper::update("Init.createIndexStudentsMajor");
        QtMyBatisHelper::update("Init.createIndexCoursesCode");
        QtMyBatisHelper::update("Init.createIndexCoursesDepartment");
        QtMyBatisHelper::update("Init.createIndexEnrollmentsStudent");
        QtMyBatisHelper::update("Init.createIndexEnrollmentsCourse");
        
        qDebug() << "数据库表创建成功";
        return true;
    } catch (const QtMyBatisException& e) {
        qCritical() << "创建数据库表失败:" << e.message();
        return false;
    }
}

void EducationService::createSampleData()
{
    try {
        // 插入示例数据
        QtMyBatisHelper::update("Init.insertSampleStudents");
        QtMyBatisHelper::update("Init.insertSampleCourses");
        QtMyBatisHelper::update("Init.insertSampleEnrollments");
        qDebug() << "示例数据创建成功";
    } catch (const QtMyBatisException& e) {
        qWarning() << "创建示例数据失败:" << e.message();
    }
}

bool EducationService::isStudentEnrolledInCourse(const QString& studentNumber, const QString& courseCode)
{
    try {
        int studentId = getStudentIdByNumber(studentNumber);
        int courseId = getCourseIdByCode(courseCode);
        
        if (studentId <= 0 || courseId <= 0) {
            return false;
        }
        
        QVariantMap params;
        params["arg1"] = studentId;
        params["arg2"] = courseId;
        
        QVariant result = QtMyBatisHelper::selectOne("Enrollment.isStudentEnrolledInCourse", params);
        return result.toInt() > 0;
    } catch (const QtMyBatisException& e) {
        qWarning() << "检查选课状态失败:" << e.message();
        return false;
    }
}

int EducationService::getStudentIdByNumber(const QString& studentNumber)
{
    Student student = getStudentByNumber(studentNumber);
    return student.id();
}

int EducationService::getCourseIdByCode(const QString& courseCode)
{
    Course course = getCourseByCode(courseCode);
    return course.id();
}

// 业务操作实现
bool EducationService::transferStudent(const QString& studentNumber, const QString& newMajor, int newGrade)
{
    if (!m_initialized) return false;
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            // 查找学生
            Student student = getStudentByNumber(studentNumber);
            if (!student.isValid()) {
                emit errorOccurred("学生不存在: " + studentNumber);
                return false;
            }
            
            // 更新学生信息
            student.setMajor(newMajor);
            student.setGrade(newGrade);
            
            // 保存更改
            return updateStudent(student);
        } catch (const QtMyBatisException& e) {
            emit errorOccurred("转专业失败: " + e.message());
            return false;
        }
    });
}

bool EducationService::graduateStudents(const QStringList& studentNumbers)
{
    if (!m_initialized) return false;
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            for (const QString& studentNumber : studentNumbers) {
                Student student = getStudentByNumber(studentNumber);
                if (student.isValid()) {
                    student.setStatus(Student::Status::GRADUATED);
                    if (!updateStudent(student)) {
                        emit errorOccurred("更新学生状态失败: " + studentNumber);
                        return false;
                    }
                }
            }
            return true;
        } catch (const QtMyBatisException& e) {
            emit errorOccurred("批量毕业操作失败: " + e.message());
            return false;
        }
    });
}

bool EducationService::createCourseSchedule(const QString& semester, const QList<Course>& courses)
{
    if (!m_initialized) return false;
    
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        try {
            for (const Course& course : courses) {
                Course newCourse = course;
                newCourse.setSemester(semester);
                if (!addCourse(newCourse)) {
                    emit errorOccurred("添加课程失败: " + course.courseCode());
                    return false;
                }
            }
            return true;
        } catch (const QtMyBatisException& e) {
            emit errorOccurred("创建课程安排失败: " + e.message());
            return false;
        }
    });
}

QList<EducationService::EnrollmentInfo> EducationService::getStudentEnrollments(const QString& studentNumber)
{
    if (!m_initialized) return {};
    
    try {
        int studentId = getStudentIdByNumber(studentNumber);
        if (studentId <= 0) return {};
        
        QVariantMap params;
        params["arg1"] = studentId;
        QVariantList results = QtMyBatisHelper::selectList("Enrollment.findStudentEnrollmentsWithDetails", params);
        
        QList<EnrollmentInfo> enrollments;
        for (const QVariant& result : results) {
            QVariantMap map = result.toMap();
            EnrollmentInfo info;
            info.enrollmentId = map["id"].toInt();
            info.student = getStudentById(studentId);
            info.course = getCourseById(map["course_id"].toInt());
            info.enrollmentDate = map["enrollment_date"].toString();
            info.grade = map["grade"].toDouble();
            info.status = map["status"].toString();
            enrollments.append(info);
        }
        return enrollments;
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("查询学生选课记录失败: " + e.message());
        return {};
    }
}

QList<EducationService::EnrollmentInfo> EducationService::getCourseEnrollments(const QString& courseCode)
{
    if (!m_initialized) return {};
    
    try {
        int courseId = getCourseIdByCode(courseCode);
        if (courseId <= 0) return {};
        
        QVariantMap params;
        params["arg1"] = courseId;
        QVariantList results = QtMyBatisHelper::selectList("Enrollment.findCourseEnrollmentsWithDetails", params);
        
        QList<EnrollmentInfo> enrollments;
        for (const QVariant& result : results) {
            QVariantMap map = result.toMap();
            EnrollmentInfo info;
            info.enrollmentId = map["id"].toInt();
            info.student = getStudentById(map["student_id"].toInt());
            info.course = getCourseById(courseId);
            info.enrollmentDate = map["enrollment_date"].toString();
            info.grade = map["grade"].toDouble();
            info.status = map["status"].toString();
            enrollments.append(info);
        }
        return enrollments;
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("查询课程选课记录失败: " + e.message());
        return {};
    }
}

QVariantList EducationService::getEnrollmentStatistics()
{
    if (!m_initialized) return {};
    
    try {
        return QtMyBatisHelper::selectList("Enrollment.countByStatus");
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("获取选课统计失败: " + e.message());
        return {};
    }
}

QVariantList EducationService::getGradeDistribution()
{
    if (!m_initialized) return {};
    
    try {
        return QtMyBatisHelper::selectList("Enrollment.getGradeDistribution");
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("获取成绩分布失败: " + e.message());
        return {};
    }
}

QList<Student> EducationService::getTopStudents(int limit)
{
    if (!m_initialized) return {};
    
    try {
        QVariantMap params;
        params["arg1"] = 80.0; // 最低GPA要求
        params["arg2"] = limit;
        QVariantList results = QtMyBatisHelper::selectList("Enrollment.getTopPerformingStudents", params);
        
        QList<Student> students;
        for (const QVariant& result : results) {
            QVariantMap map = result.toMap();
            Student student;
            student.setId(map["student_id"].toInt());
            student.setStudentNumber(map["student_number"].toString());
            student.setName(map["name"].toString());
            students.append(student);
        }
        return students;
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("获取优秀学生失败: " + e.message());
        return {};
    }
}

QList<Course> EducationService::getPopularCourses(int limit)
{
    if (!m_initialized) return {};
    
    try {
        QVariantMap params;
        params["arg1"] = limit;
        QVariantList results = QtMyBatisHelper::selectList("Enrollment.getMostPopularCourses", params);
        
        QList<Course> courses;
        for (const QVariant& result : results) {
            QVariantMap map = result.toMap();
            Course course;
            course.setId(map["course_id"].toInt());
            course.setCourseCode(map["course_code"].toString());
            course.setName(map["name"].toString());
            courses.append(course);
        }
        return courses;
    } catch (const QtMyBatisException& e) {
        emit errorOccurred("获取热门课程失败: " + e.message());
        return {};
    }
} 