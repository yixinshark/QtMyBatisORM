# Qt MyBatis ORM 简化使用指南

## 概述

Qt MyBatis ORM 现在提供了极简的使用方式，通过新的配置格式和静态工具类，让数据库操作变得前所未有的简单。

## 核心特性

### 🚀 一行初始化
```cpp
QtMyBatisHelper::initialize(":/config/database.json");
```

### 🔧 统一配置格式
所有配置集中在一个JSON文件中，包括数据库连接和SQL文件列表。

### 📝 SQL调试日志
通过`debug: true`自动打印所有SQL执行详情，包括参数和耗时。

### 🎯 静态接口
完全隐藏ORM和Session概念，业务层只需调用静态方法。

### 🔄 自动事务管理
内置RAII确保Session正确关闭，连接及时归还。

## 快速开始

### 1. 配置文件格式

创建 `:/config/database.json`：

```json
{
    "database": {
        "debug": true,
        "type": "QMYSQL",
        "host": "127.0.0.1",
        "port": 3306,
        "database_name": "hello",
        "username": "test",
        "password": "11111111",
        "max_wait_time": 5000,
        "max_connection_count": 5,
        "min_connection_count": 2,
        "max_idle_time": 300,
        "cache_enabled": true,
        "max_cache_size": 1000,
        "cache_expire_time": 600,
        "sql_files": [
            ":/sql/person.sql",
            ":/sql/info.sql"
        ]
    }
}
```

#### 配置字段说明

##### 基础数据库配置
| 字段 | 类型 | 必填 | 默认值 | 说明 |
|-----|------|------|--------|------|
| `debug` | boolean | 否 | false | 是否开启SQL调试日志 |
| `type` | string | 是 | "QSQLITE" | 数据库类型：QMYSQL/QSQLITE |
| `host` | string | 否 | "localhost" | 数据库主机地址 |
| `port` | number | 否 | 3306 | 数据库端口 |
| `database_name` | string | 是 | - | 数据库名或文件路径 |
| `username` | string | 否 | - | 数据库用户名 |
| `password` | string | 否 | - | 数据库密码 |
| `sql_files` | array | 是 | [] | SQL映射文件列表 |

##### 连接池配置
| 字段 | 类型 | 必填 | 默认值 | 推荐值 | 说明 |
|-----|------|------|--------|--------|------|
| `max_connection_count` | number | 否 | 10 | 5-20 | **最大连接数**<br/>• 低并发应用：5-10<br/>• 中等并发：10-15<br/>• 高并发应用：15-50<br/>• 不要超过数据库最大连接限制 |
| `min_connection_count` | number | 否 | 2 | 2-5 | **最小连接数**<br/>• 保持的最少连接数<br/>• 建议设为 max_connection_count 的 20-50%<br/>• 避免频繁创建销毁连接的开销 |
| `max_idle_time` | number | 否 | 300 | 300-600 | **最大空闲时间（秒）**<br/>• 连接空闲超时后自动关闭<br/>• 短时间应用：180-300秒<br/>• 长期运行：300-600秒 |
| `max_wait_time` | number | 否 | 5000 | 3000-10000 | **连接等待超时（毫秒）**<br/>• 获取连接的最大等待时间<br/>• 快速响应：3000-5000ms<br/>• 宽松设置：5000-10000ms |

##### 缓存配置
| 字段 | 类型 | 必填 | 默认值 | 推荐值 | 说明 |
|-----|------|------|--------|--------|------|
| `cache_enabled` | boolean | 否 | true | true | **是否启用缓存**<br/>• 读多写少的应用：建议启用<br/>• 实时性要求高：可考虑禁用<br/>• 开发调试阶段：可临时禁用 |
| `max_cache_size` | number | 否 | 1000 | 500-5000 | **最大缓存条目数量**<br/>• 小型应用：500-1000<br/>• 中型应用：1000-3000<br/>• 大型应用：3000-10000<br/>• 考虑可用内存限制 |
| `cache_expire_time` | number | 否 | 600 | 300-1800 | **缓存过期时间（秒）**<br/>• 数据变化频繁：300-600秒<br/>• 数据相对稳定：600-1800秒<br/>• 静态数据：1800-3600秒<br/>• 设为0表示永不过期（不推荐） |

##### 场景化推荐配置

**🏠 小型应用（个人项目、工具软件）**
```json
{
    "max_connection_count": 5,
    "min_connection_count": 2,
    "max_idle_time": 300,
    "max_wait_time": 5000,
    "cache_enabled": true,
    "max_cache_size": 500,
    "cache_expire_time": 600
}
```

**🏢 中型应用（企业内部系统）**
```json
{
    "max_connection_count": 15,
    "min_connection_count": 5,
    "max_idle_time": 450,
    "max_wait_time": 8000,
    "cache_enabled": true,
    "max_cache_size": 2000,
    "cache_expire_time": 900
}
```

**🏭 大型应用（高并发Web服务）**
```json
{
    "max_connection_count": 30,
    "min_connection_count": 10,
    "max_idle_time": 600,
    "max_wait_time": 10000,
    "cache_enabled": true,
    "max_cache_size": 5000,
    "cache_expire_time": 1200
}
```

**⚡ 高性能应用（实时系统）**
```json
{
    "max_connection_count": 50,
    "min_connection_count": 15,
    "max_idle_time": 300,
    "max_wait_time": 3000,
    "cache_enabled": true,
    "max_cache_size": 10000,
    "cache_expire_time": 300
}
```

### 2. SQL映射文件

创建 `:/sql/person.sql`：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<sqls namespace="Person">
    <define id="fields">id, name, age, email</define>

    <sql id="createTable">
        CREATE TABLE IF NOT EXISTS `person` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `name` VARCHAR(100) NOT NULL,
            `age` INT NOT NULL,
            `email` VARCHAR(255) DEFAULT NULL
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    </sql>

    <sql id="findAll">
        SELECT <include defineId="fields"/> FROM person
    </sql>

    <sql id="findById">
        SELECT <include defineId="fields"/> FROM person WHERE id = %1
    </sql>

    <sql id="insert">
        INSERT INTO person (name, age, email)
        VALUES (:name, :age, :email)
    </sql>

    <sql id="update">
        UPDATE person SET name = :name, age = :age, email = :email WHERE id = :id
    </sql>

    <sql id="deleteById">
        DELETE FROM person WHERE id = %1
    </sql>
</sqls>
```

### 3. 数据模型类

```cpp
class Person {
public:
    int id;
    QString name;
    int age;
    QString email;
    
    Person() : id(0), age(0) {}
    
    QVariantMap toMap() const {
        QVariantMap map;
        if (id > 0) map["id"] = id;
        map["name"] = name;
        map["age"] = age;
        map["email"] = email;
        return map;
    }
    
    static Person fromMap(const QVariantMap& map) {
        Person person;
        person.id = map["id"].toInt();
        person.name = map["name"].toString();
        person.age = map["age"].toInt();
        person.email = map["email"].toString();
        return person;
    }
};
```

### 4. DAO类实现

```cpp
class PersonDao {
public:
    static bool createTable() {
        try {
            return QtMyBatisHelper::execute("Person.createTable") >= 0;
        } catch (const QtMyBatisException& e) {
            qWarning() << "创建表失败:" << e.message();
            return false;
        }
    }
    
    static QList<Person> findAll() {
        try {
            QVariantList results = QtMyBatisHelper::selectList("Person.findAll");
            QList<Person> persons;
            for (const auto& result : results) {
                persons.append(Person::fromMap(result.toMap()));
            }
            return persons;
        } catch (const QtMyBatisException& e) {
            qWarning() << "查询失败:" << e.message();
            return {};
        }
    }
    
    static Person findById(int id) {
        try {
            QVariantMap params;
            params["arg1"] = id;
            QVariant result = QtMyBatisHelper::selectOne("Person.findById", params);
            return Person::fromMap(result.toMap());
        } catch (const QtMyBatisException& e) {
            qWarning() << "根据ID查询失败:" << e.message();
            return Person();
        }
    }
    
    static bool insert(const Person& person) {
        try {
            return QtMyBatisHelper::insert("Person.insert", person.toMap()) > 0;
        } catch (const QtMyBatisException& e) {
            qWarning() << "插入失败:" << e.message();
            return false;
        }
    }
    
    static bool update(const Person& person) {
        try {
            return QtMyBatisHelper::update("Person.update", person.toMap()) > 0;
        } catch (const QtMyBatisException& e) {
            qWarning() << "更新失败:" << e.message();
            return false;
        }
    }
    
    static bool deleteById(int id) {
        try {
            QVariantMap params;
            params["arg1"] = id;
            return QtMyBatisHelper::remove("Person.deleteById", params) > 0;
        } catch (const QtMyBatisException& e) {
            qWarning() << "删除失败:" << e.message();
            return false;
        }
    }
    
    // 批量操作示例
    static bool insertBatch(const QList<Person>& persons) {
        try {
            QList<QVariantMap> paramsList;
            for (const auto& person : persons) {
                paramsList.append(person.toMap());
            }
            return QtMyBatisHelper::batchInsert("Person.insert", paramsList) == persons.size();
        } catch (const QtMyBatisException& e) {
            qWarning() << "批量插入失败:" << e.message();
            return false;
        }
    }
    
    // 事务操作示例
    static bool transferData(const QList<Person>& oldPersons, const QList<Person>& newPersons) {
        return QtMyBatisHelper::executeInTransaction([&]() -> bool {
            try {
                // 删除旧数据
                for (const auto& person : oldPersons) {
                    if (!deleteById(person.id)) {
                        return false;
                    }
                }
                
                // 插入新数据
                for (const auto& person : newPersons) {
                    if (!insert(person)) {
                        return false;
                    }
                }
                
                return true;
            } catch (...) {
                return false;
            }
        });
    }
};
```

### 5. 主程序使用

```cpp
#include <QCoreApplication>
#include <QtMyBatisORM/qtmybatishelper.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 1. 一行初始化
    if (!QtMyBatisHelper::initialize(":/config/database.json")) {
        qCritical() << "初始化失败";
        return 1;
    }
    
    // 2. 开启调试（可选）
    QtMyBatisHelper::enableDebugMode(true);
    
    // 3. 使用DAO进行数据库操作
    PersonDao::createTable();
    
    Person person;
    person.name = "张三";
    person.age = 25;
    person.email = "zhangsan@example.com";
    
    if (PersonDao::insert(person)) {
        qDebug() << "插入成功";
    }
    
    auto allPersons = PersonDao::findAll();
    qDebug() << "共有" << allPersons.size() << "条记录";
    
    // 4. 程序结束前清理
    QtMyBatisHelper::shutdown();
    
    return 0;
}
```

## 调试日志示例

当 `debug: true` 时，会自动输出详细的SQL执行日志：

```
[QtMyBatisORM DEBUG] execute: Person.createTable 耗时:15ms
[QtMyBatisORM DEBUG] insert: Person.insert 参数:[name=张三, age=25, email=zhangsan@example.com] 结果:[1] 耗时:3ms
[QtMyBatisORM DEBUG] selectList: Person.findAll 结果:[返回1条记录] 耗时:2ms
[QtMyBatisHelper] 开始事务
[QtMyBatisORM DEBUG] remove: Person.deleteById 参数:[arg1=1] 结果:[1] 耗时:2ms
[QtMyBatisORM DEBUG] insert: Person.insert 参数:[name=李四, age=30, email=lisi@example.com] 结果:[1] 耗时:1ms
[QtMyBatisHelper] 事务提交成功
```

## 缓存功能

QtMyBatisORM 内置了强大的缓存功能，可以显著提高查询性能。

### 缓存配置

在 `database.json` 中启用和配置缓存：

```json
{
    "database": {
        "cache_enabled": true,        // 启用缓存
        "max_cache_size": 1000,       // 最大缓存条目数
        "cache_expire_time": 600      // 缓存过期时间（秒）
    }
}
```

### 缓存特性

1. **自动缓存**: SELECT 查询结果会自动缓存，相同查询直接返回缓存结果
2. **智能失效**: INSERT/UPDATE/DELETE 操作会自动清理相关表的缓存
3. **LRU策略**: 当缓存达到最大容量时，自动清理最少使用的条目
4. **过期清理**: 缓存条目会在设定时间后自动过期

### 缓存示例

```cpp
// 第一次查询，从数据库获取数据并缓存
auto person1 = QtMyBatisHelper::selectOne("Person.findById", {{"arg1", 1}});
// [DEBUG] 查询数据库并缓存结果

// 第二次相同查询，直接从缓存获取
auto person2 = QtMyBatisHelper::selectOne("Person.findById", {{"arg1", 1}});
// [DEBUG] 从缓存获取结果，耗时几乎为0

// 更新操作会自动清理相关缓存
QtMyBatisHelper::update("Person.update", {{"id", 1}, {"name", "新名字"}});
// [DEBUG] 自动清理 Person 表相关的缓存

// 再次查询时会重新从数据库获取最新数据
auto person3 = QtMyBatisHelper::selectOne("Person.findById", {{"arg1", 1}});
// [DEBUG] 缓存已失效，重新查询数据库
```

### 缓存调试

启用调试模式时，可以看到缓存的工作状态：

```
[QtMyBatisORM DEBUG] selectOne: Person.findById [缓存未命中] 查询数据库 耗时:5ms
[QtMyBatisORM DEBUG] selectOne: Person.findById [缓存命中] 从缓存获取 耗时:0ms
[QtMyBatisORM DEBUG] update: Person.update 清理缓存:Person表 耗时:2ms
```

## API 参考

### QtMyBatisHelper 静态方法

#### 初始化和管理
- `bool initialize(const QString& configResourcePath)` - 初始化ORM
- `void shutdown()` - 关闭ORM并清理资源
- `bool isInitialized()` - 检查是否已初始化
- `void enableDebugMode(bool enabled = true)` - 开启/关闭调试模式
- `bool isDebugMode()` - 检查调试模式状态

#### 基础CRUD操作
- `QVariant selectOne(const QString& statementId, const QVariantMap& parameters = {})` - 查询单条记录
- `QVariantList selectList(const QString& statementId, const QVariantMap& parameters = {})` - 查询多条记录
- `int insert(const QString& statementId, const QVariantMap& parameters = {})` - 插入记录
- `int update(const QString& statementId, const QVariantMap& parameters = {})` - 更新记录
- `int remove(const QString& statementId, const QVariantMap& parameters = {})` - 删除记录
- `int execute(const QString& sql, const QVariantMap& parameters = {})` - 执行SQL语句

#### 批量操作
- `int batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList)` - 批量插入
- `int batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList)` - 批量更新
- `int batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList)` - 批量删除

#### 事务操作
- `bool executeInTransaction(std::function<bool()> operation)` - 执行事务（Lambda方式）
- `bool executeInTransaction(std::function<bool(QSharedPointer<Session>)> operation)` - 执行事务（Session方式）

## 最佳实践

### 1. 错误处理
```cpp
try {
    auto result = QtMyBatisHelper::selectOne("Person.findById", params);
    // 处理结果
} catch (const QtMyBatisException& e) {
    qWarning() << "数据库操作失败:" << e.message();
    // 错误处理逻辑
}
```

### 2. 事务使用
```cpp
bool success = QtMyBatisHelper::executeInTransaction([&]() -> bool {
    // 在这里执行多个数据库操作
    // 返回 true 提交事务，false 回滚事务
    return allOperationsSuccessful;
});
```

### 3. 参数传递
```cpp
// 位置参数（%1, %2, %3...）
QVariantMap params;
params["arg1"] = value1;
params["arg2"] = value2;

// 命名参数（:name, :age, :email...）
QVariantMap params;
params["name"] = "张三";
params["age"] = 25;
params["email"] = "zhangsan@example.com";
```

### 4. 调试技巧
- 开发阶段设置 `debug: true` 查看SQL执行详情
- 生产环境设置 `debug: false` 提升性能
- 使用 `QtMyBatisHelper::enableDebugMode()` 动态控制调试模式

## 从旧版本迁移

### 配置文件迁移
```javascript
// 旧格式
{
    "driverName": "QMYSQL",
    "hostName": "localhost",
    // ...
}

// 新格式
{
    "database": {
        "type": "QMYSQL",
        "host": "localhost",
        // ...
    }
}
```

### 代码迁移
```cpp
// 旧方式
auto orm = QtMyBatisORM::create(":/config/database.json", {":/sql/person.sql"});
auto session = orm->openSession();
auto result = session->selectOne("Person.findById", params);
orm->closeSession(session);

// 新方式
QtMyBatisHelper::initialize(":/config/database.json");
auto result = QtMyBatisHelper::selectOne("Person.findById", params);
```

## 性能优化建议

1. **连接池配置**: 根据并发需求调整 `max_connection_count` 和 `min_connection_count`
2. **缓存配置**: 
   - 启用缓存 (`cache_enabled: true`) 以提高查询性能
   - 根据内存限制调整 `max_cache_size`
   - 根据数据更新频率设置合适的 `cache_expire_time`
3. **批量操作**: 大量数据操作时使用 `batchInsert/batchUpdate/batchRemove`
4. **事务使用**: 相关操作组合在一个事务中执行
5. **调试模式**: 生产环境关闭调试模式
6. **SQL优化**: 合理使用索引和查询条件
7. **空闲连接管理**: 设置合适的 `max_idle_time` 避免连接占用过久

## 故障排除

### 常见问题

1. **初始化失败**
   - 检查配置文件路径是否正确
   - 确认配置文件已添加到Qt资源系统
   - 验证JSON格式是否正确

2. **SQL执行失败**
   - 开启调试模式查看具体SQL语句
   - 检查参数映射是否正确
   - 验证数据库连接是否正常

3. **事务回滚**
   - 检查事务函数返回值
   - 确认异常处理逻辑
   - 验证数据库支持事务

### 日志分析
```
[QtMyBatisHelper DEBUG] selectOne: Person.findById 参数:[arg1=1] 结果:[{"id":1,"name":"张三"}] 耗时:2ms [Session已自动关闭]
```

日志包含：
- 操作类型 (selectOne)
- SQL语句ID (Person.findById)
- 传入参数 (arg1=1)
- 返回结果概要
- 执行耗时 (2ms)
- Session状态确认 