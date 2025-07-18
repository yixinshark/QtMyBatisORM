# QtMyBatisORM

QtMyBatisORM是一个基于Qt6框架的ORM（对象关系映射）组件，借鉴了Java MyBatis的设计理念和架构模式。该组件提供了简洁的数据库操作接口，支持XML配置的SQL映射，具备连接池管理和缓存机制，专门为Qt应用程序设计。

## 特性

- 简洁的API接口，易于使用
- 支持XML配置的SQL映射
- 支持JSON格式的数据库配置
- 高效的数据库连接池管理
- 灵活的缓存机制（LRU策略）
- 支持MySQL和SQLite等数据库
- 支持事务管理
- 与Qt6框架深度集成
- 跨平台兼容性（Linux、Windows、macOS）
- 模块化设计
- 线程安全
- 异常安全

## 系统要求

- Qt 6.0或更高版本
- C++17或更高版本
- CMake 3.16或更高版本
- 支持的数据库：MySQL、SQLite

## 项目结构

```
QtMyBatisORM/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目说明文档
├── include/                # 头文件目录
│   └── QtMyBatisORM/
│       ├── DataModels.h    # 基础数据结构
│       ├── qtmybatisorm.h  # 主入口类
│       ├── configurationmanager.h
│       ├── jsonconfigparser.h
│       ├── xmlmapperparser.h
│       ├── connectionpool.h
│       ├── cachemanager.h
│       ├── session.h
│       ├── sessionfactory.h
│       ├── executor.h
│       ├── mapperregistry.h
│       ├── mapperproxy.h
│       └── ...
├── src/                    # 源代码目录
│   ├── core/              # 核心功能模块
│   ├── config/            # 配置解析模块
│   ├── cache/             # 缓存管理模块
│   ├── pool/              # 连接池模块
│   ├── mapper/            # 映射器模块
│   ├── exception/         # 异常处理模块
│   └── qtmybatisorm.cpp   # 主入口实现
├── tests/                  # 测试代码目录
├── examples/               # 示例代码目录
└── docs/                   # 文档目录
```

## 核心组件

### 1. 配置管理
- **ConfigurationManager**: 单例配置管理器
- **JSONConfigParser**: JSON配置文件解析器
- **XMLMapperParser**: XML映射文件解析器

### 2. 数据库连接
- **ConnectionPool**: 数据库连接池，支持连接复用和自动清理
- **Session**: 数据库会话，提供CRUD操作接口
- **SessionFactory**: 会话工厂，管理Session生命周期

### 3. SQL执行引擎
- **Executor**: SQL执行器，处理SQL语句执行
- **StatementHandler**: SQL语句处理器
- **ParameterHandler**: 参数处理器
- **ResultHandler**: 结果处理器
- **DynamicSqlProcessor**: 动态SQL处理器

### 4. 映射系统
- **MapperRegistry**: Mapper注册表
- **MapperProxy**: 动态代理，实现DAO接口

### 5. 缓存系统
- **CacheManager**: 缓存管理器，支持LRU策略和过期清理

### 6. 异常处理
- **QtMyBatisException**: 基础异常类
- **ConfigurationException**: 配置异常
- **SqlExecutionException**: SQL执行异常
- **ConnectionException**: 连接异常
- **MappingException**: 映射异常

## 安装

### 使用CMake构建

```bash
# 克隆仓库
git clone https://github.com/yixinshark/QtMyBatisORM.git
cd QtMyBatisORM

# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 构建
cmake --build .

# 安装（可选）
cmake --install .
```

### 构建选项

- `BUILD_SHARED_LIBS`: 构建共享库（默认：ON）
- `BUILD_TESTING`: 构建测试（默认：ON）
- `BUILD_EXAMPLES`: 构建示例（默认：OFF）
- `BUILD_DOCS`: 构建文档（默认：OFF）

示例：

```bash
cmake -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=ON ..
```

### 运行测试

```bash
# 运行所有测试
ctest

# 运行特定测试
./tests/test_qtmybatisorm_standalone
```

## 快速入门

### 基本用法

```cpp
#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatisorm.h>
#include <QtMyBatisORM/session.h>

using namespace QtMyBatisORM;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 创建一个内存数据库的ORM实例
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createDefault();
    
    // 获取会话
    QSharedPointer<Session> session = orm->openSession();
    
    // 执行一些基本操作
    try {
        // 创建表
        session->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
        
        // 插入数据
        QVariantMap user;
        user["name"] = "John Doe";
        user["email"] = "john@example.com";
        
        session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", user);
        
        // 查询数据
        QVariantList users = session->selectList("SELECT * FROM users");
        qDebug() << "Found" << users.size() << "users";
        
        for (const QVariant& userVar : users) {
            QVariantMap user = userVar.toMap();
            qDebug() << "User:" << user["id"].toInt() << user["name"].toString() << user["email"].toString();
        }
    } catch (const QtMyBatisException& e) {
        qCritical() << "Error:" << e.message();
        return 1;
    }
    
    // 关闭会话
    orm->closeSession(session);
    
    return 0;
}
```

### 使用事务

```cpp
// 开始事务
session->beginTransaction();

try {
    // 执行多个操作
    session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", user1);
    session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", user2);
    
    // 提交事务
    session->commit();
} catch (const QtMyBatisException& e) {
    // 发生错误时回滚事务
    session->rollback();
    qCritical() << "Error:" << e.message();
}
```

### 使用XML映射

1. 创建XML映射文件 (user_mapper.xml):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="UserMapper">
    <select id="findAll" resultType="User">
        SELECT * FROM users
    </select>
    
    <select id="findById" parameterType="int" resultType="User">
        SELECT * FROM users WHERE id = :id
    </select>
    
    <insert id="insert" parameterType="User">
        INSERT INTO users (name, email) VALUES (:name, :email)
    </insert>
    
    <update id="update" parameterType="User">
        UPDATE users SET name = :name, email = :email WHERE id = :id
    </update>
    
    <delete id="delete" parameterType="int">
        DELETE FROM users WHERE id = :id
    </delete>
</mapper>
```

2. 加载XML映射文件:

```cpp
// 初始化ORM时加载映射文件
QStringList mapperPaths = {":/mappers/user_mapper.xml"};
QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::create(":/config/database.json", mapperPaths);
```

3. 使用映射的SQL:

```cpp
// 使用映射的SQL语句
QVariantList users = session->selectList("UserMapper.findAll");

// 使用带参数的映射SQL语句
QVariantMap params;
params["id"] = 1;
QVariant user = session->selectOne("UserMapper.findById", params);
```

### 使用数据库连接池

连接池是自动管理的，您只需要在配置中设置连接池参数:

```json
{
    "driverName": "QMYSQL",
    "hostName": "localhost",
    "port": 3306,
    "databaseName": "mydb",
    "userName": "root",
    "password": "password",
    "maxConnections": 10,
    "minConnections": 2,
    "maxIdleTime": 300
}
```

### 使用缓存

缓存也是自动管理的，您只需要在配置中启用缓存:

```json
{
    "driverName": "QSQLITE",
    "databaseName": ":memory:",
    "cacheEnabled": true,
    "maxCacheSize": 1000,
    "cacheExpireTime": 600
}
```

在XML映射中，您可以为特定查询启用或禁用缓存:

```xml
<select id="findAll" resultType="User" useCache="true">
    SELECT * FROM users
</select>
```

## 高级用法

### 自定义数据库配置

```cpp
// 创建自定义数据库配置
DatabaseConfig config;
config.driverName = "QMYSQL";
config.hostName = "localhost";
config.port = 3306;
config.databaseName = "mydb";
config.userName = "user";
config.password = "password";
config.maxConnections = 20;
config.minConnections = 5;
config.maxIdleTime = 600;
config.cacheEnabled = true;
config.maxCacheSize = 2000;
config.cacheExpireTime = 1200;

// 使用自定义配置初始化ORM
QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createWithConfig(config);
```

### 动态SQL

```cpp
// 构建动态SQL
QString sql = "SELECT * FROM products WHERE 1=1";
QVariantMap params;

if (!category.isEmpty()) {
    sql += " AND category = :category";
    params["category"] = category;
}

if (minPrice > 0) {
    sql += " AND price >= :minPrice";
    params["minPrice"] = minPrice;
}

if (maxPrice > 0) {
    sql += " AND price <= :maxPrice";
    params["maxPrice"] = maxPrice;
}

// 执行动态SQL
QVariantList products = session->selectList(sql, params);
```

### 使用Mapper代理

1. 定义Mapper接口:

```cpp
// UserMapper.h
class UserMapper
{
public:
    virtual ~UserMapper() {}
    
    virtual QVariantList findAll() = 0;
    virtual QVariant findById(int id) = 0;
    virtual int insert(const QVariantMap& user) = 0;
    virtual int update(const QVariantMap& user) = 0;
    virtual int deleteById(int id) = 0;
};
```

2. 获取Mapper实例:

```cpp
// 获取Mapper实例
UserMapper* userMapper = session->getMapper<UserMapper>();

// 使用Mapper
QVariantList users = userMapper->findAll();
QVariant user = userMapper->findById(1);
```

## 示例代码

### 基本示例

```cpp
// 创建一个内存数据库的ORM实例
auto orm = QtMyBatisORM::QtMyBatisORM::createDefault();

// 获取会话
auto session = orm->openSession();

// 创建表
session->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");

// 插入数据
QVariantMap user;
user["name"] = "John Doe";
user["email"] = "john@example.com";
session->execute("INSERT INTO users (name, email) VALUES (:name, :email)", user);

// 查询数据
QVariantList users = session->selectList("SELECT * FROM users");
```

### SQLite示例

```cpp
// 创建SQLite数据库
auto orm = QtMyBatisORM::QtMyBatisORM::createSQLite("mydb.sqlite");

// 获取会话
auto session = orm->openSession();

// 开始事务
session->beginTransaction();

try {
    // 执行操作
    session->execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
    
    // 提交事务
    session->commit();
} catch (const QtMyBatisException& e) {
    // 回滚事务
    session->rollback();
    qCritical() << "Error:" << e.message();
}
```

### MySQL示例

```cpp
// 创建MySQL数据库连接
auto orm = QtMyBatisORM::QtMyBatisORM::createMySQL(
    "localhost", 3306, "mydb", "user", "password");

// 获取会话
auto session = orm->openSession();

// 执行查询
QVariantList results = session->selectList("SELECT * FROM users WHERE age > :age", {{"age", 18}});
```

## 开发进度

### ✅ 已完成
- [x] **任务1: 设置项目结构和核心接口**
- [x] **任务2: 实现配置管理组件**
  - [x] 2.1 创建配置数据模型和JSON解析器
  - [x] 2.2 实现XML映射文件解析器
  - [x] 2.3 实现ConfigurationManager单例类
- [x] **任务3: 实现数据库连接池**
  - [x] 3.1 创建基础连接管理
  - [x] 3.2 实现连接池核心功能
  - [x] 3.3 添加连接池监控和异常处理
- [x] **任务4: 实现SQL执行引擎**
  - [x] 4.1 创建基础SQL执行器
  - [x] 4.2 实现参数处理器
  - [x] 4.3 实现结果处理器
  - [x] 4.4 实现动态SQL处理器
- [x] **任务5: 实现会话管理**
  - [x] 5.1 创建Session核心类
  - [x] 5.2 实现事务管理
  - [x] 5.3 实现SessionFactory
- [x] **任务6: 实现Mapper代理系统**
  - [x] 6.1 创建MapperRegistry
  - [x] 6.2 实现MapperProxy动态代理
  - [x] 6.3 集成Mapper到Session
- [x] **任务7: 实现缓存管理**
  - [x] 7.1 创建基础缓存管理器
  - [x] 7.2 实现LRU缓存策略
  - [x] 7.3 集成缓存到SQL执行流程
- [x] **任务8: 实现异常处理和错误管理**
  - [x] 8.1 创建异常体系
  - [x] 8.2 集成异常处理到各组件
- [x] **任务9: 创建主入口和API封装**
  - [x] 9.1 实现QtMyBatisORM主类
  - [x] 9.2 实现便捷的工厂方法
- [x] **任务10: 完善CMake构建和安装配置**
  - [x] 10.1 优化CMake构建脚本
  - [x] 10.2 创建使用示例和文档

### 🚧 进行中
- [ ] **任务11: 集成测试和性能优化**
  - [ ] 11.1 创建完整的集成测试套件
  - [ ] 11.2 性能优化和最终调试

## 贡献指南

1. Fork 本项目 [QtMyBatisORM](https://github.com/yixinshark/QtMyBatisORM)
2. 创建特性分支 (`git checkout -b feature/your-feature-name`)
3. 提交更改 (`git commit -m 'Add your feature description'`)
4. 推送到分支 (`git push origin feature/your-feature-name`)
5. 打开 [Pull Request](https://github.com/yixinshark/QtMyBatisORM/pulls)

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 联系方式

- 项目链接: [https://github.com/yixinshark/QtMyBatisORM](https://github.com/yixinshark/QtMyBatisORM)
- 问题反馈: [Issues](https://github.com/yixinshark/QtMyBatisORM/issues)