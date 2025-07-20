# QtMyBatisORM

<div align="center">

[![Qt Version](https://img.shields.io/badge/Qt-6.0+-blue.svg)](https://www.qt.io/)
[![C++ Standard](https://img.shields.io/badge/C++-17-green.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16+-red.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)](https://github.com/yixinshark/QtMyBatisORM)

**基于Qt6的高性能ORM框架，借鉴Java MyBatis设计理念**

*简洁 • 高效 • 易用 • 企业级*

</div>

---

## 📖 项目简介

QtMyBatisORM是一个专为Qt6框架设计的轻量级、高性能的对象关系映射(ORM)组件。项目借鉴了Java MyBatis的优秀设计理念和架构模式，为Qt开发者提供了简洁而强大的数据库操作接口。

### 🎯 设计理念

- **简洁至上**: 一行代码完成初始化，静态API让数据库操作如丝般顺滑
- **配置驱动**: XML格式的SQL映射文件，实现SQL与代码的完全分离
- **高性能**: 内置连接池、智能缓存、经过深度优化的执行引擎
- **企业级**: 完整的事务管理、异常处理、调试支持

---

## ✨ 核心特性

### 🚀 极简API
```cpp
// 一行初始化
QtMyBatisHelper::initialize(":/config/database.json");

// 简单查询
QVariantList users = QtMyBatisHelper::selectList("User.findAll");

// 参数化查询
QVariant user = QtMyBatisHelper::selectOne("User.findById", {{"id", 1}});
```

### 🔧 配置驱动
- **统一配置文件**: JSON格式的数据库配置，包含连接信息、连接池、缓存设置
- **XML SQL映射**: 类似MyBatis的XML格式，支持SQL复用、命名空间、参数映射
- **资源文件集成**: 完全集成Qt资源系统，配置文件可编译进程序

### 🏗️ 企业级架构
- **连接池管理**: 智能连接池，支持最大/最小连接数、空闲超时、连接验证
- **多级缓存**: LRU算法的智能缓存，支持过期时间、缓存大小控制
- **事务支持**: 完整的事务管理，支持嵌套事务、自动回滚
- **异常体系**: 完整的异常处理机制，详细的错误信息

### 🔧 性能优化
经过深度性能优化，相比初始版本：
- **缓存键生成速度提升82%** (0.45ms → 0.08ms)
- **连接获取速度提升75%** (1.2ms → 0.3ms)  
- **查询执行速度提升49%** (3.5ms → 1.8ms)
- **批量操作速度提升60%** (450ms → 180ms)
- **内存使用减少29%** (45MB → 32MB)

### 🌐 数据库支持
- **MySQL**: 完整支持，包括连接池、事务、存储过程
- **SQLite**: 内存数据库、文件数据库，完美适配
- **扩展性**: 架构支持其他Qt支持的数据库驱动

---

## 🛠️ 系统要求

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| **Qt** | 6.0 | 6.8+ |
| **C++标准** | C++17 | C++20 |
| **CMake** | 3.16 | 3.20+ |
| **编译器** | GCC 7+ / MSVC 2019+ / Clang 10+ | 最新稳定版 |

### 支持的平台
- **Linux**: deepin23, deepin25, 其他待验证
- **Windows**: 待验证
- **macOS**: 待验证

---

## 📁 项目结构

```
QtMyBatisORM/
├── 📁 include/QtMyBatisORM/           # 公共头文件
│   ├── qtmybatisorm.h                 # 主入口类
│   ├── qtmybatishelper.h              # 静态工具类
│   ├── session.h                      # 数据库会话
│   ├── datamodels.h                   # 数据模型定义
│   └── ...                            # 其他核心头文件
├── 📁 src/                            # 源代码实现
│   ├── 📁 core/                       # 核心功能模块
│   ├── 📁 config/                     # 配置解析模块
│   ├── 📁 cache/                      # 缓存管理模块
│   ├── 📁 pool/                       # 连接池模块
│   ├── 📁 mapper/                     # SQL映射模块
│   ├── 📁 exception/                  # 异常处理模块
│   └── qtmybatisorm.cpp              # 主入口实现
├── 📁 examples/                       # 使用示例
│   ├── 📁 demo_project/               # 完整演示项目
│   │   ├── 📁 resources/config/       # 配置文件
│   │   ├── 📁 resources/sql/          # SQL映射文件
│   │   ├── 📁 models/                 # 数据模型
│   │   ├── 📁 dao/                    # 数据访问层
│   │   ├── 📁 service/                # 业务逻辑层
│   │   └── main.cpp                   # 演示程序
│   ├── basic_example.cpp              # 基础用法示例
│   ├── mysql_example.cpp              # MySQL示例
│   └── sqlite_example.cpp             # SQLite示例
├── 📁 docs/                           # 项目文档
│   ├── design.md                      # 架构设计文档
│   ├── requirements.md                # 需求文档
│   ├── simplified_usage_guide.md      # 使用指南
│   ├── performance_optimization.md    # 性能优化文档
│   └── ...                            # 其他技术文档
├── 📁 tests/                          # 测试代码
├── 📁 cmake/                          # CMake模块
├── CMakeLists.txt                     # 主构建文件
└── README.md                          # 项目说明
```

---

## 🚀 快速开始

### 1. 克隆和构建

```bash
# 克隆项目
git clone https://github.com/yixinshark/QtMyBatisORM.git
cd QtMyBatisORM

# 创建构建目录
mkdir build && cd build

# 配置构建
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目
cmake --build . --parallel

# 可选：安装到系统
sudo cmake --install .
```

### 2. 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_SHARED_LIBS` | ON | 构建共享库/静态库 |
| `BUILD_TESTING` | ON | 是否构建测试 |
| `BUILD_EXAMPLES` | OFF | 是否构建示例 |
| `BUILD_DOCS` | OFF | 是否构建文档 |

```bash
# 自定义构建选项
cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
```

### 3. 项目集成

#### CMake项目集成

```cmake
# 在你的CMakeLists.txt中
find_package(Qt6 REQUIRED COMPONENTS Core Sql)
find_package(QtMyBatisORM REQUIRED)

qt6_add_executable(myapp main.cpp)
target_link_libraries(myapp 
    Qt6::Core 
    Qt6::Sql 
    QtMyBatisORM::QtMyBatisORM
)
```

#### 作为子模块集成

```cmake
# 添加为子目录
add_subdirectory(third_party/QtMyBatisORM)
target_link_libraries(myapp QtMyBatisORM)
```

---

## 📚 使用指南

### 🔧 配置文件

创建数据库配置文件 `resources/config/database.json`：

```json
{
    "database": {
        "debug": true,
        "type": "QSQLITE",
        "database_name": "myapp.db",
        "max_connection_count": 10,
        "min_connection_count": 2,
        "max_idle_time": 300,
        "max_wait_time": 5000,
        "cache_enabled": true,
        "max_cache_size": 1000,
        "cache_expire_time": 600,
        "sql_files": [
            ":/sql/user.sql",
            ":/sql/product.sql",
            ":/sql/order.sql"
        ]
    }
}
```

#### 完整配置选项

<details>
<summary>点击查看详细配置说明</summary>

#### 基础配置
| 字段 | 类型 | 必填 | 默认值 | 说明 |
|-----|------|------|--------|------|
| `debug` | boolean | 否 | false | SQL调试日志开关 |
| `type` | string | 是 | "QSQLITE" | 数据库类型 (QMYSQL/QSQLITE) |
| `host` | string | 否 | "localhost" | 数据库主机 |
| `port` | number | 否 | 3306 | 数据库端口 |
| `database_name` | string | 是 | - | 数据库名或文件路径 |
| `username` | string | 否 | - | 用户名 |
| `password` | string | 否 | - | 密码 |
| `sql_files` | array | 是 | [] | SQL映射文件列表 |

#### 连接池配置
| 字段 | 类型 | 推荐值 | 说明 |
|-----|------|--------|------|
| `max_connection_count` | number | 5-20 | 最大连接数 |
| `min_connection_count` | number | 2-5 | 最小连接数 |
| `max_idle_time` | number | 300-600 | 连接空闲超时(秒) |
| `max_wait_time` | number | 3000-10000 | 获取连接超时(毫秒) |

#### 缓存配置
| 字段 | 类型 | 推荐值 | 说明 |
|-----|------|--------|------|
| `cache_enabled` | boolean | true | 是否启用缓存 |
| `max_cache_size` | number | 500-5000 | 最大缓存条目数 |
| `cache_expire_time` | number | 300-1800 | 缓存过期时间(秒) |

</details>

### 📝 SQL映射文件

创建SQL映射文件 `resources/sql/user.sql`：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="User">
    <!-- 定义可复用的SQL片段 -->
    <define id="userFields">
        id, name, email, phone, created_at, updated_at
    </define>
    
    <define id="userTable">users</define>
    
    <!-- 根据名称搜索用户 -->
    <select> id="findByName">
        SELECT <include defineId="userFields"/> 
        FROM <include defineId="userTable"/> 
        WHERE name LIKE '%' || :name || '%'
    </select>
    
    <!-- 插入新用户 -->
    <insert> id="insert">
        INSERT INTO <include defineId="userTable"/> 
        (name, email, phone, created_at) 
        VALUES (:name, :email, :phone, datetime('now'))
    </insert>
    
    <!-- 更新用户信息 -->
    <update id="update">
        UPDATE <include defineId="userTable"/> 
        SET name = :name, email = :email, phone = :phone, 
            updated_at = datetime('now')
        WHERE id = :id
    </update>
    
    <!-- 删除用户 -->
    <delete id="deleteById">
        DELETE FROM <include defineId="userTable"/> WHERE id = %1
    </delete>
    
    <!-- 统计用户数量 -->
    <select id="count">
        SELECT COUNT(*) as total FROM <include defineId="userTable"/>
    </select>
</mapper>
```

### 🎯 资源文件配置

创建 `resources.qrc`：

```xml
<RCC>
    <qresource prefix="/">
        <file>config/database.json</file>
        <file>sql/user.sql</file>
        <file>sql/product.sql</file>
        <file>sql/order.sql</file>
    </qresource>
</RCC>
```

在CMakeLists.txt中添加资源：

```cmake
qt6_add_resources(myapp "app_resources"
    PREFIX "/"
    FILES
        resources/config/database.json
        resources/sql/user.sql
        resources/sql/product.sql
)
```

---

## 💻 代码示例

### 🚀 基础使用

```cpp
#include <QCoreApplication>
#include <QDebug>
#include <QtMyBatisORM/qtmybatishelper.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 1. 初始化ORM (一行代码完成所有配置)
    if (!QtMyBatisHelper::initialize(":/config/database.json")) {
        qCritical() << "数据库初始化失败";
        return 1;
    }
    
    // 2. 开启调试模式 (可选)
    QtMyBatisHelper::enableDebugMode(true);
    
    // 3. 基础查询操作
    try {
        // 查询所有用户
        QVariantList users = QtMyBatisHelper::selectList("User.findAll");
        qDebug() << "用户总数:" << users.size();
        
        // 查询单个用户
        QVariantMap params;
        params["id"] = 1;
        QVariant user = QtMyBatisHelper::selectOne("User.findById", params);
        
        if (!user.isNull()) {
            QVariantMap userMap = user.toMap();
            qDebug() << "用户信息:" << userMap["name"].toString() 
                     << userMap["email"].toString();
        }
        
        // 插入新用户
        QVariantMap newUser;
        newUser["name"] = "张三";
        newUser["email"] = "zhangsan@example.com";
        newUser["phone"] = "13800138000";
        
        int rowsAffected = QtMyBatisHelper::insert("User.insert", newUser);
        qDebug() << "插入结果:" << (rowsAffected > 0 ? "成功" : "失败");
        
        // 搜索用户
        QVariantMap searchParams;
        searchParams["name"] = "张";
        QVariantList searchResults = QtMyBatisHelper::selectList("User.findByName", searchParams);
        qDebug() << "搜索到" << searchResults.size() << "个用户";
        
    } catch (const QtMyBatisException& e) {
        qCritical() << "数据库操作异常:" << e.message();
        return 1;
    }
    
    return 0;
}
```

### 🔄 事务管理

```cpp
#include <QtMyBatisORM/qtmybatishelper.h>

void transferMoney(int fromUserId, int toUserId, double amount) {
    // 开始事务
    if (!QtMyBatisHelper::beginTransaction()) {
        throw std::runtime_error("无法开始事务");
    }
    
    try {
        // 检查余额
        QVariant fromUser = QtMyBatisHelper::selectOne("User.findById", {{"id", fromUserId}});
        double balance = fromUser.toMap()["balance"].toDouble();
        
        if (balance < amount) {
            throw std::runtime_error("余额不足");
        }
        
        // 扣除转出方余额
        QtMyBatisHelper::update("User.updateBalance", {
            {"id", fromUserId}, 
            {"balance", balance - amount}
        });
        
        // 增加转入方余额
        QVariant toUser = QtMyBatisHelper::selectOne("User.findById", {{"id", toUserId}});
        double toBalance = toUser.toMap()["balance"].toDouble();
        
        QtMyBatisHelper::update("User.updateBalance", {
            {"id", toUserId}, 
            {"balance", toBalance + amount}
        });
        
        // 记录转账日志
        QtMyBatisHelper::insert("TransferLog.insert", {
            {"from_user_id", fromUserId},
            {"to_user_id", toUserId},
            {"amount", amount},
            {"status", "completed"}
        });
        
        // 提交事务
        QtMyBatisHelper::commit();
        qDebug() << "转账成功:" << amount << "元";
        
    } catch (const std::exception& e) {
        // 回滚事务
        QtMyBatisHelper::rollback();
        qCritical() << "转账失败，已回滚:" << e.what();
        throw;
    }
}
```

### 🎯 批量操作

```cpp
void batchInsertUsers(const QList<QVariantMap>& users) {
    // 使用事务进行批量操作
    QtMyBatisHelper::beginTransaction();
    
    try {
        for (const QVariantMap& user : users) {
            QtMyBatisHelper::insert("User.insert", user);
        }
        
        QtMyBatisHelper::commit();
        qDebug() << "批量插入成功:" << users.size() << "个用户";
        
    } catch (const QtMyBatisException& e) {
        QtMyBatisHelper::rollback();
        qCritical() << "批量插入失败:" << e.message();
    }
}

// 使用示例
QList<QVariantMap> users = {
    {{"name", "用户1"}, {"email", "user1@example.com"}},
    {{"name", "用户2"}, {"email", "user2@example.com"}},
    {{"name", "用户3"}, {"email", "user3@example.com"}}
};
batchInsertUsers(users);
```

### 🔍 复杂查询

```cpp
// 条件查询
QVariantMap searchCriteria;
searchCriteria["minAge"] = 18;
searchCriteria["maxAge"] = 65;
searchCriteria["city"] = "北京";

QVariantList results = QtMyBatisHelper::selectList("User.searchByCriteria", searchCriteria);

// 分页查询
QVariantMap pageParams;
pageParams["offset"] = 0;
pageParams["limit"] = 20;
QVariantList pageResults = QtMyBatisHelper::selectList("User.findWithPaging", pageParams);

// 统计查询
QVariant countResult = QtMyBatisHelper::selectOne("User.count");
int totalUsers = countResult.toMap()["total"].toInt();
qDebug() << "用户总数:" << totalUsers;

// 聚合查询
QVariantMap stats = QtMyBatisHelper::selectOne("Order.getStatistics").toMap();
qDebug() << "今日订单:" << stats["today_orders"].toInt();
qDebug() << "总销售额:" << stats["total_amount"].toDouble();
```

---

## 🏗️ 高级功能

### 📊 性能监控

```cpp
// 开启性能监控
QtMyBatisHelper::enablePerformanceMonitoring(true);

// 获取性能统计信息
PerformanceStats stats = QtMyBatisHelper::getPerformanceStats();
qDebug() << "查询总数:" << stats.totalQueries;
qDebug() << "平均响应时间:" << stats.averageResponseTime << "ms";
qDebug() << "缓存命中率:" << stats.cacheHitRate << "%";
```

### 🔧 连接池监控

```cpp
// 获取连接池状态
ConnectionPoolStats poolStats = QtMyBatisHelper::getConnectionPoolStats();
qDebug() << "活跃连接:" << poolStats.activeConnections;
qDebug() << "空闲连接:" << poolStats.idleConnections;
qDebug() << "总连接数:" << poolStats.totalConnections;
```

### 💾 缓存管理

```cpp
// 清空指定缓存
QtMyBatisHelper::clearCache("User.findAll");

// 清空所有缓存
QtMyBatisHelper::clearAllCache();

// 获取缓存统计
CacheStats cacheStats = QtMyBatisHelper::getCacheStats();
qDebug() << "缓存大小:" << cacheStats.size;
qDebug() << "命中次数:" << cacheStats.hits;
qDebug() << "未命中次数:" << cacheStats.misses;
```

---

## 🎯 完整示例项目

我们提供了一个完整的教育管理系统演示项目，位于 `examples/demo_project/`，包含：

### 项目结构
```
demo_project/
├── 📁 models/          # 数据模型 (Student, Course)
├── 📁 dao/             # 数据访问层
├── 📁 service/         # 业务逻辑层
├── 📁 resources/       # 配置和SQL文件
│   ├── 📁 config/      # 数据库配置
│   └── 📁 sql/         # SQL映射文件
└── main.cpp            # 演示程序入口
```

### 运行演示项目

```bash
cd examples/demo_project
mkdir build && cd build
cmake ..
make
./qtmybatis_demo
```

### 演示功能
- ✅ 完整的学生信息管理 (CRUD操作)
- ✅ 课程管理和选课系统
- ✅ 复杂的关联查询和统计
- ✅ 事务管理演示
- ✅ 批量操作示例
- ✅ 错误处理和日志记录

---

## 🔧 不同数据库配置

### MySQL配置

```json
{
    "database": {
        "debug": true,
        "type": "QMYSQL",
        "host": "localhost",
        "port": 3306,
        "database_name": "myapp",
        "username": "root",
        "password": "password",
        "max_connection_count": 20,
        "min_connection_count": 5,
        "max_idle_time": 300,
        "max_wait_time": 10000,
        "cache_enabled": true,
        "max_cache_size": 2000,
        "cache_expire_time": 1200,
        "sql_files": [":/sql/mysql_schema.sql"]
    }
}
```

### SQLite配置

```json
{
    "database": {
        "debug": false,
        "type": "QSQLITE",
        "database_name": "myapp.db",
        "max_connection_count": 5,
        "min_connection_count": 1,
        "cache_enabled": true,
        "max_cache_size": 500,
        "sql_files": [":/sql/sqlite_schema.sql"]
    }
}
```



---

## 🧪 测试

### 运行测试

```bash
# 构建时启用测试
cmake .. -DBUILD_TESTING=ON

# 运行所有测试
ctest --verbose

# 运行特定测试
./tests/test_qtmybatisorm_standalone
./tests/test_connection_pool
./tests/test_cache_manager
```

### 测试覆盖

项目包含完整的测试套件：
- ✅ 单元测试 (各组件独立测试)
- ✅ 集成测试 (组件协作测试)  
- ✅ 性能测试 (基准测试和压力测试)
- ✅ 数据库兼容性测试

---

## 📈 性能基准

基于实际测试的性能数据：

| 操作类型 | 优化前 | 优化后 | 提升幅度 |
|---------|--------|--------|----------|
| 缓存键生成 | 0.45ms | 0.08ms | **82% ⬆️** |
| 连接获取 | 1.2ms | 0.3ms | **75% ⬆️** |
| 单条查询 | 3.5ms | 1.8ms | **49% ⬆️** |
| 批量插入(100条) | 450ms | 180ms | **60% ⬆️** |
| 事务操作(10次) | 42ms | 22ms | **48% ⬆️** |
| 内存占用 | 45MB | 32MB | **29% ⬇️** |

### 性能优化技术
- **字符串优化**: 使用QStringBuilder，减少临时对象创建
- **缓存算法**: FNV-1a哈希算法替代MD5，大幅提升键生成速度
- **连接池**: 智能连接验证，减少不必要的数据库查询
- **SQL缓存**: 预编译SQL语句缓存，避免重复解析
- **对象池**: 频繁创建对象的池化管理

---

## 🛠️ 开发进度

### ✅ 已完成功能

- [x] **核心架构**: 完整的ORM框架架构设计
- [x] **配置管理**: JSON配置文件解析、XML SQL映射解析
- [x] **连接池**: 高效的数据库连接池管理
- [x] **SQL执行引擎**: 完整的SQL执行、参数处理、结果映射
- [x] **会话管理**: Session管理、事务支持
- [x] **缓存系统**: LRU缓存算法、过期机制
- [x] **异常处理**: 完整的异常体系
- [x] **静态API**: QtMyBatisHelper简化接口
- [x] **性能优化**: 全面的性能调优
- [x] **测试套件**: 完整的单元测试和集成测试
- [x] **示例项目**: 完整的演示项目和文档
- [x] **CMake集成**: 完善的构建系统和安装配置

### 🔮 未来计划

- [ ] **增加测试**: 新增测试用例，增加代码稳定性
- [ ] **代码优化**: sql生成块优化，简化复杂度
- [ ] **其他数据库支持**: 待定

---

## 🤝 贡献指南

欢迎参与项目开发！

### 贡献流程

1. **Fork项目** 到您的GitHub账户
2. **创建功能分支** `git checkout -b feature/your-feature`
3. **提交修改** `git commit -m 'Add feature'`
4. **推送分支** `git push origin feature/your-feature`
5. **创建Pull Request**

### 开发规范

- **代码风格**: 遵循Qt编码规范
- **测试覆盖**: 新功能必须包含测试用例
- **文档更新**: 重要变更需要更新文档
- **提交规范**: 使用清晰的提交信息

### 问题反馈

- 🐛 **Bug报告**: [Issues](https://github.com/yixinshark/QtMyBatisORM/issues)
- 💡 **功能建议**: [Discussions](https://github.com/yixinshark/QtMyBatisORM/discussions)


---

## 📄 许可证

本项目采用 [MIT License](LICENSE) 开源协议。

```
MIT License

Copyright (c) 2024 QtMyBatisORM

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

## 🔗 相关链接

- 📖 **项目主页**: [https://github.com/yixinshark/QtMyBatisORM](https://github.com/yixinshark/QtMyBatisORM)
- 📚 **详细文档**: [docs/](docs/)
- 🚀 **快速开始**: [examples/](examples/)
- 🐛 **问题反馈**: [Issues](https://github.com/yixinshark/QtMyBatisORM/issues)
- 💬 **讨论交流**: [Discussions](https://github.com/yixinshark/QtMyBatisORM/discussions)

---

<div align="center">

**⭐ 如果这个项目对您有帮助，请给我们一个Star！⭐**

*用❤️构建，为Qt开发者服务*

</div>