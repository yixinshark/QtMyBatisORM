# QtMyBatisORM 集成测试套件

本文档描述了 QtMyBatisORM 的集成测试套件，包括测试范围、测试方法和如何运行测试。

## 测试套件概述

QtMyBatisORM 的集成测试套件旨在验证组件之间的交互和整个系统的功能。测试套件包括以下几个主要部分：

1. **端到端集成测试** - 测试整个 ORM 系统的功能，从配置到数据库操作
2. **缓存集成测试** - 测试缓存系统与 SQL 执行引擎的集成
3. **性能基准测试** - 测试系统在各种条件下的性能表现
4. **多数据库集成测试** - 测试系统与不同数据库后端的兼容性
5. **会话-映射器集成测试** - 测试会话与映射器代理系统的集成
6. **异常集成测试** - 测试异常处理系统在各组件中的集成

## 运行测试

### 运行所有测试

```bash
cd build
make check
```

或者使用详细输出：

```bash
cd build
make check_verbose
```

### 运行单个测试

```bash
cd build
./tests/test_end_to_end_integration_standalone
```

## 测试内容详解

### 1. 端到端集成测试 (test_end_to_end_integration.cpp)

端到端测试验证整个 ORM 系统的功能，包括：

- 基本的 CRUD 操作
- 事务管理（提交和回滚）
- XML 映射文件加载和使用
- Mapper 代理系统
- 缓存集成
- 连接池使用
- 并发访问
- 错误处理

测试场景包括：

- 产品和订单管理系统模拟
- 多线程并发操作
- 事务一致性验证
- 错误条件处理

### 2. 缓存集成测试 (test_cache_integration.cpp)

缓存集成测试验证缓存系统与 SQL 执行引擎的集成，包括：

- 基础缓存功能（命中、未命中）
- 缓存键生成
- 缓存失效机制（插入、更新、删除）
- 事务中的缓存行为
- 并发缓存访问
- 边界条件测试（空结果、禁用缓存）

### 3. 性能基准测试 (test_performance_benchmark.cpp)

性能基准测试评估系统在各种条件下的性能表现，包括：

- 基本查询性能
- 缓存查询性能
- 批量插入和更新性能
- 事务性能
- 并发查询性能
- 连接池性能
- 内存使用情况
- 不同数据量下的可扩展性
- 不同缓存配置的影响

### 4. 多数据库集成测试 (test_multi_database_integration.cpp)

多数据库集成测试验证系统与不同数据库后端的兼容性，包括：

- SQLite 基本操作
- MySQL 基本操作（如果可用）
- 跨数据库兼容性
- 数据库特定功能
- 多连接管理
- 连接切换

### 5. 会话-映射器集成测试 (test_session_mapper_integration.cpp)

会话-映射器集成测试验证会话与映射器代理系统的集成，包括：

- 基本映射器获取
- 映射器生命周期绑定
- 映射器方法调用
- 多会话映射器实例

### 6. 异常集成测试 (test_exception_integration.cpp)

异常集成测试验证异常处理系统在各组件中的集成，包括：

- 配置异常
- SQL 执行异常
- 连接异常
- 映射异常
- 事务异常

## 测试覆盖范围

集成测试套件覆盖了 QtMyBatisORM 的以下核心功能：

1. **配置管理**
   - JSON 配置解析
   - XML 映射文件解析
   - 配置验证

2. **数据库连接**
   - 连接池管理
   - 连接获取和释放
   - 连接监控

3. **会话管理**
   - 会话创建和关闭
   - 事务管理
   - SQL 执行

4. **映射系统**
   - 映射注册
   - 动态代理
   - 方法调用

5. **缓存系统**
   - 缓存命中和未命中
   - 缓存失效
   - 缓存统计

6. **异常处理**
   - 异常类型
   - 异常传播
   - 错误恢复

## 添加新测试

要添加新的集成测试，请按照以下步骤操作：

1. 创建测试运行文件（例如 `run_new_integration_test.cpp`）：

```cpp
#include <QtTest>
#include "test_new_integration.cpp"

QTEST_MAIN(TestNewIntegration)
```

2. 创建测试实现文件（例如 `test_new_integration.cpp`）：

```cpp
#include <QtTest/QtTest>
#include "QtMyBatisORM/qtmybatisorm.h"

using namespace QtMyBatisORM;

class TestNewIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 测试方法
    void testSomething();
};

// 实现测试方法...

#include "test_new_integration.moc"
```

3. 更新 `tests/CMakeLists.txt` 文件：

```cmake
add_qtmybatis_test(test_new_integration_standalone run_new_integration_test.cpp)
```

4. 将测试文件添加到主测试可执行文件：

```cmake
set(TEST_SOURCES
    ...
    test_new_integration.cpp
)
```

## 测试最佳实践

1. **独立性** - 每个测试应该是独立的，不依赖于其他测试的状态
2. **清理** - 测试应该清理它创建的任何资源
3. **可重复性** - 测试应该在任何环境中都能产生相同的结果
4. **边界条件** - 测试应该包括边界条件和错误情况
5. **性能考虑** - 性能测试应该使用基准测试框架并报告统计数据

## 常见问题

### 测试失败时如何调试？

1. 使用详细输出运行测试：

```bash
./tests/test_end_to_end_integration_standalone -v2
```

2. 检查测试日志输出
3. 使用调试器运行测试：

```bash
gdb --args ./tests/test_end_to_end_integration_standalone
```

### 如何跳过特定测试？

在测试方法中使用 `QSKIP` 宏：

```cpp
void TestMultiDatabaseIntegration::testMySQLConnection()
{
    if (!m_mysqlAvailable) {
        QSKIP("MySQL不可用，跳过测试");
    }
    
    // 测试代码...
}
```

### 如何添加测试资源？

1. 将资源文件放在 `tests/resources` 目录中
2. 在测试中使用相对路径访问资源：

```cpp
QFile file("resources/test_config.json");
```

## 结论

QtMyBatisORM 的集成测试套件提供了全面的测试覆盖，确保系统的各个组件能够正确地协同工作。通过运行这些测试，可以验证系统的功能、性能和稳定性。