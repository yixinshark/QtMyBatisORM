# QtMyBatisORM 演示项目

这是一个完整的QtMyBatisORM演示项目，展示了如何在实际应用中使用QtMyBatisORM进行数据库操作。

## 📋 项目概述

本演示项目是一个教育管理系统，包含学生管理、课程管理和选课管理等功能。通过这个项目，您可以学习到：

- 📁 **项目结构设计** - 标准的MVC架构
- 🗄️ **数据库设计** - 完整的表结构和关系
- 🔧 **配置管理** - JSON配置文件的最佳实践
- 📝 **SQL映射** - XML格式的SQL文件组织
- 💻 **代码组织** - Model、DAO、Service层的设计
- 🔄 **事务管理** - 复杂业务逻辑的事务处理
- ✅ **数据验证** - 完整的数据校验机制

## 🏗️ 项目结构

```
demo_project/
├── CMakeLists.txt                  # CMake构建配置
├── main.cpp                        # 主程序入口
├── README.md                       # 项目说明文档
├── models/                         # 数据模型层
│   ├── student.h/.cpp             # 学生模型
│   └── course.h/.cpp              # 课程模型
├── dao/                           # 数据访问层
│   ├── studentdao.h/.cpp          # 学生DAO
│   └── coursedao.h/.cpp           # 课程DAO
├── service/                       # 业务逻辑层
│   └── educationservice.h/.cpp   # 教育管理服务
└── resources/                     # 资源文件
    ├── demo_resources.qrc         # Qt资源文件
    ├── config/
    │   └── database.json          # 数据库配置
    └── sql/
        ├── init.sql               # 初始化SQL
        ├── student.sql            # 学生相关SQL
        ├── course.sql             # 课程相关SQL
        └── enrollment.sql         # 选课相关SQL
```

## 🚀 快速开始

### 环境要求

- Qt 6.0 或更高版本
- C++17 或更高版本
- CMake 3.16 或更高版本
- QtMyBatisORM 库

### 编译运行

1. **克隆或复制demo项目**
```bash
# 假设您已经有了QtMyBatisORM项目
cd QtMyBatisORM/examples/demo_project
```

2. **创建构建目录**
```bash
mkdir build && cd build
```

3. **配置CMake**
```bash
cmake ..
```

4. **编译项目**
```bash
cmake --build .
```

5. **运行演示程序**
```bash
./qtmybatis_demo
```

## 📖 核心功能演示

### 1. 一行初始化

```cpp
// 只需一行代码即可初始化整个ORM系统
QtMyBatisHelper::initialize(":/config/database.json");
```

### 2. 统一配置管理

```json
{
    "database": {
        "debug": true,
        "type": "QSQLITE",
        "database_name": "education_demo.db",
        "sql_files": [
            ":/sql/init.sql",
            ":/sql/student.sql",
            ":/sql/course.sql",
            ":/sql/enrollment.sql"
        ]
    }
}
```

### 3. 简洁的数据操作

```cpp
// 查询所有学生
QVariantList students = QtMyBatisHelper::selectList("Student.findAll");

// 插入新学生
QtMyBatisHelper::insert("Student.insert", student.toMap());

// 事务操作
QtMyBatisHelper::executeInTransaction([&]() -> bool {
    // 复杂的业务逻辑
    return true;
});
```

## 🎯 设计模式和最佳实践

### 1. 模型层设计

**Student模型**展示了完整的数据模型设计：

- ✅ 完整的字段定义和访问器
- ✅ 数据转换方法（toMap/fromMap）
- ✅ 数据验证和业务规则
- ✅ 调试输出支持
- ✅ 枚举类型的正确使用

```cpp
class Student {
public:
    enum class Status { ACTIVE, INACTIVE, GRADUATED, DROPPED, DELETED };
    enum class Gender { UNKNOWN, MALE, FEMALE };
    
    // 标准的getter/setter
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    // 数据转换
    QVariantMap toMap() const;
    static Student fromMap(const QVariantMap& map);
    
    // 数据验证
    QString validateData() const;
};
```

### 2. DAO层设计

**StudentDao**展示了完整的数据访问层设计：

- ✅ 基础CRUD操作
- ✅ 条件查询和分页
- ✅ 复杂查询和统计
- ✅ 批量操作
- ✅ 错误处理和日志
- ✅ 信号通知机制

```cpp
class StudentDao : public QObject {
    Q_OBJECT
public:
    // 基础操作
    QList<Student> findAll();
    bool insert(const Student& student);
    
    // 条件查询
    QList<Student> findByMajor(const QString& major);
    QList<Student> findWithPagination(int limit, int offset);
    
    // 批量操作
    bool batchInsert(const QList<Student>& students);
    
    // 业务操作（带事务）
    bool transferStudent(const QString& studentNumber, 
                        const QString& newMajor, int newGrade);
signals:
    void studentInserted(const Student& student);
    void errorOccurred(const QString& error);
};
```

### 3. 服务层设计

**EducationService**展示了业务逻辑层的设计：

- ✅ 复杂业务操作的封装
- ✅ 跨表事务处理
- ✅ 数据校验和业务规则
- ✅ 统计分析功能
- ✅ 错误处理和回滚

```cpp
class EducationService : public QObject {
public:
    // 复杂业务操作
    bool enrollStudent(const QString& studentNumber, const QString& courseCode);
    bool transferStudent(const QString& studentNumber, 
                        const QString& newMajor, int newGrade);
    
    // 统计分析
    Statistics getOverallStatistics();
    
    // 数据验证
    QString validateEnrollment(const QString& studentNumber, 
                              const QString& courseCode);
};
```

## 📝 SQL文件组织

### 命名空间管理

每个SQL文件使用独立的命名空间：

```xml
<!-- student.sql -->
<sqls namespace="Student">
    <sql id="findAll">SELECT * FROM students WHERE status = 'ACTIVE'</sql>
    <sql id="insert">INSERT INTO students (...) VALUES (...)</sql>
</sqls>

<!-- course.sql -->
<sqls namespace="Course">
    <sql id="findAll">SELECT * FROM courses WHERE status = 'ACTIVE'</sql>
    <sql id="insert">INSERT INTO courses (...) VALUES (...)</sql>
</sqls>
```

### 参数化查询

支持多种参数传递方式：

```xml
<!-- 位置参数 -->
<sql id="findById">
    SELECT * FROM students WHERE id = %1
</sql>

<!-- 命名参数 -->
<sql id="insert">
    INSERT INTO students (name, email) VALUES (:name, :email)
</sql>

<!-- 复杂条件 -->
<sql id="findByConditions">
    SELECT * FROM students 
    WHERE major LIKE '%1%' AND grade = %2 
    ORDER BY student_number LIMIT %3 OFFSET %4
</sql>
```

### 代码复用

使用`<define>`和`<include>`实现SQL代码复用：

```xml
<define id="fields">
    id, student_number, name, gender, birth_date, major, grade,
    phone, email, address, enrollment_date, status, created_at, updated_at
</define>

<sql id="findAll">
    SELECT <include defineId="fields"/> FROM students
    WHERE status = 'ACTIVE' ORDER BY student_number
</sql>
```

## 🔍 调试和监控

### SQL调试日志

开启调试模式后，所有SQL执行都会有详细日志：

```cpp
QtMyBatisHelper::enableDebugMode(true);
```

输出示例：
```
[DEBUG] Executing SQL: Student.findAll
[DEBUG] SQL: SELECT id, student_number, name, ... FROM students WHERE status = 'ACTIVE'
[DEBUG] Parameters: {}
[DEBUG] Execution time: 15ms
[DEBUG] Result count: 5
```

### 错误处理

完整的错误处理机制：

```cpp
try {
    QVariantList results = QtMyBatisHelper::selectList("Student.findAll");
    // 处理结果
} catch (const QtMyBatisException& e) {
    qWarning() << "数据库操作失败:" << e.message();
    // 错误处理逻辑
}
```

## 🎓 学习要点

### 1. 项目初始化流程

```cpp
// 1. 初始化QtMyBatisORM（读取配置文件和SQL文件）
QtMyBatisHelper::initialize(":/config/database.json");

// 2. 开启调试模式（可选）
QtMyBatisHelper::enableDebugMode(true);

// 3. 创建业务服务
EducationService service;
service.initialize();
```

### 2. 数据模型设计原则

- **完整性**: 包含所有必要的字段和方法
- **验证性**: 提供数据验证机制
- **转换性**: 实现与QVariantMap的双向转换
- **可调试**: 支持调试输出
- **类型安全**: 使用枚举类型提高类型安全

### 3. DAO设计原则

- **单一职责**: 每个DAO只负责一个实体
- **异常安全**: 完整的错误处理
- **信号通知**: 重要操作的事件通知
- **事务支持**: 复杂操作的事务管理
- **性能优化**: 批量操作和分页查询

### 4. 服务层设计原则

- **业务封装**: 将复杂业务逻辑封装在服务层
- **事务管理**: 跨表操作使用事务
- **数据验证**: 业务规则验证
- **错误处理**: 统一的错误处理机制

## 🔧 扩展指南

### 添加新的实体

1. **创建模型类**
```cpp
// models/teacher.h
class Teacher {
    // 完整的模型定义
};
```

2. **创建DAO类**
```cpp
// dao/teacherdao.h
class TeacherDao : public QObject {
    // DAO方法定义
};
```

3. **创建SQL文件**
```xml
<!-- resources/sql/teacher.sql -->
<sqls namespace="Teacher">
    <sql id="findAll">SELECT * FROM teachers</sql>
</sqls>
```

4. **更新配置文件**
```json
{
    "database": {
        "sql_files": [
            ":/sql/teacher.sql"
        ]
    }
}
```

### 添加新的业务功能

1. **在服务层添加方法**
```cpp
bool EducationService::assignTeacherToCourse(int teacherId, int courseId) {
    return QtMyBatisHelper::executeInTransaction([=]() -> bool {
        // 复杂的业务逻辑
        return true;
    });
}
```

2. **添加相应的SQL**
```xml
<sql id="assignTeacher">
    UPDATE courses SET teacher_id = :teacher_id WHERE id = :course_id
</sql>
```

## 📚 参考资料

- [QtMyBatisORM 官方文档](../../README.md)
- [SQL映射文件格式说明](../../docs/sql_mapping.md)
- [配置文件说明](../../docs/configuration.md)
- [最佳实践指南](../../docs/best_practices.md)

## 🤝 贡献指南

欢迎提交Issue和Pull Request来改进这个演示项目！

## 📄 许可证

本项目使用与QtMyBatisORM相同的许可证。 