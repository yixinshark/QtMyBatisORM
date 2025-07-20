# Qt MyBatis ORM 调试指南

## 概述

本文档记录了在开发过程中遇到的典型问题及其调试和解决过程，为后续开发和维护提供参考。

## 案例研究：SimpleHelperTest 测试失败

### 问题描述

在运行 `SimpleHelperTest` 测试时遇到以下错误：

```bash
FAIL!: TestSimpleHelper::testSimpleCRUD() Caught unhandled exception
terminate called after throwing an instance of 'QtMyBatisORM::SessionException'
what(): Failed to execute SQL: %1Failed to execute update: Parameter count mismatch. SQL: INSERT INTO test (name) VALUES (:name)
```

### 调试思路和过程

#### 第一步：理解错误信息

初始错误信息显示：
- 异常类型：`QtMyBatisORM::SessionException`
- 错误内容：`Parameter count mismatch`
- 涉及的SQL：`INSERT INTO test (name) VALUES (:name)`

**初步分析**：看起来是SQL参数绑定的问题，命名参数`:name`没有正确匹配。

#### 第二步：添加调试信息

为了更好地理解问题，我们在关键位置添加了调试输出：

```cpp
// 在 ParameterHandler::setParameters 中添加
qDebug() << "[ParameterHandler] SQL:" << sql;
qDebug() << "[ParameterHandler] Has named params:" << hasNamedParams;
qDebug() << "[ParameterHandler] Parameters:" << parameters;

// 在 bindByName 方法中添加
qDebug() << "[bindByName] Starting with SQL:" << sql;
qDebug() << "[bindByName] Found SQL parameter:" << paramName;
qDebug() << "[bindByName] Binding" << fullParamName << "=" << value;
```

#### 第三步：发现第一个问题 - 错误消息格式化

通过调试发现错误消息中有问题：`%1Failed to execute update`

**问题原因**：在 `Session` 类中使用了错误的字符串格式化方式：

```cpp
// 错误的方式
SessionException ex(
    QLatin1String("Failed to execute SQL: %1") + e.message(),
    "SESSION_EXECUTE_ERROR"
);

// 正确的方式
SessionException ex(
    QStringLiteral("Failed to execute SQL: %1").arg(e.message()),
    "SESSION_EXECUTE_ERROR"
);
```

**修复方法**：在所有相关的错误处理代码中，将字符串连接改为使用 `.arg()` 方法。

#### 第四步：深入调查参数绑定

修复错误消息后，继续调试参数绑定问题。添加了更详细的调试信息：

```cpp
// 在 Executor::updateInternal 中添加
qDebug() << "[Executor] About to execute query...";
qDebug() << "[Executor] Query SQL:" << query.lastQuery();
qDebug() << "[Executor] Bound values:" << query.boundValues();
qDebug() << "[Executor] Bound value names:" << query.boundValueNames();
```

#### 第五步：创建对照测试

为了验证Qt SQLite的参数绑定是否正常工作，创建了一个简单的测试：

```cpp
// tests/test_qt_sqlite_binding.cpp
void TestQtSqliteBinding::testNamedParameterBinding()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_named_params");
    db.setDatabaseName(":memory:");
    QVERIFY(db.open());
    
    QSqlQuery createQuery(db);
    QVERIFY(createQuery.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)"));
    
    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO test (name) VALUES (:name)");
    insertQuery.bindValue(":name", "test_value");
    
    qDebug() << "Bound values:" << insertQuery.boundValues();
    qDebug() << "Bound value names:" << insertQuery.boundValueNames();
    
    QVERIFY(insertQuery.exec());
}
```

**结果**：对照测试通过，说明Qt SQLite的参数绑定本身是正常的。

#### 第六步：发现根本问题

通过 `StatementHandler::prepare` 的调试信息发现：

```
[StatementHandler] Prepare result: false
[StatementHandler] Prepare failed: "no such table: test Unable to execute statement"
```

**关键发现**：问题不是参数绑定，而是表不存在！

**根本原因分析**：
1. 每次调用 `QtMyBatisHelper::execute()` 都会创建新的 `SessionScope`
2. 每个 `SessionScope` 从连接池获取新的数据库连接
3. 对于内存数据库 (`:memory:`)，每个新连接都创建独立的数据库实例
4. `CREATE TABLE` 在一个数据库实例中执行，`INSERT` 在另一个数据库实例中执行

### 解决方案

#### 方案一：使用事务确保操作在同一个Session中

```cpp
bool result = QtMyBatisHelper::executeInTransaction([&](QSharedPointer<Session> session) -> bool {
    // 创建表
    if (session->execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)") < 0) {
        return false;
    }
    
    // 插入数据
    QVariantMap params;
    params["0"] = "test_name";
    if (session->execute("INSERT INTO test (name) VALUES (?)", params) <= 0) {
        return false;
    }
    
    return true;
});
```

#### 方案二：配置数据库连接池（如果需要）

```json
{
    "database": {
        "type": "QSQLITE",
        "database_name": ":memory:",
        "max_connection_count": 1,  // 限制为单个连接
        "min_connection_count": 1
    }
}
```

### 调试技巧总结

#### 1. 系统性添加调试信息

```cpp
// 在关键路径添加调试输出
qDebug() << "[Component] Operation:" << details;

// 在错误处理中添加详细信息
if (!success) {
    qDebug() << "[Component] Error details:" << errorInfo;
}
```

#### 2. 创建最小化复现测试

当遇到复杂问题时，创建简单的对照测试来隔离问题：

```cpp
// 简单的Qt功能测试
void testBasicQtFeature() {
    // 测试最基本的Qt功能
    // 确定问题是在我们的代码还是Qt本身
}
```

#### 3. 逐步缩小问题范围

1. **第一层**：确定问题的大致位置（网络、数据库、逻辑等）
2. **第二层**：确定具体的组件或模块
3. **第三层**：确定具体的方法或函数
4. **第四层**：确定具体的代码行

#### 4. 检查假设

在调试过程中，我们最初假设问题是参数绑定，但实际上是数据库连接隔离。

**教训**：
- 不要过早下结论
- 验证每个假设
- 使用对照测试验证基础功能

### 常见问题模式

#### 1. 内存数据库连接隔离

**问题**：每个新连接创建独立的内存数据库实例

**解决方案**：
- 使用事务确保操作在同一个Session中
- 或者限制连接池大小为1

#### 2. 字符串格式化错误

**问题**：使用 `+` 连接包含 `%1` 的字符串

**解决方案**：使用 `.arg()` 方法

```cpp
// 错误
QString("Error: %1") + errorMsg

// 正确
QString("Error: %1").arg(errorMsg)
```

#### 3. 异常信息不完整

**问题**：异常信息缺少关键调试信息

**解决方案**：在异常中包含更多上下文信息

```cpp
throw SqlExecutionException(
    QStringLiteral("Failed to execute SQL: %1. SQL: %2")
    .arg(query.lastError().text())
    .arg(processedSql)
);
```

### 调试工具和技术

#### 1. 编译时调试

```cpp
#ifdef DEBUG
#define DEBUG_LOG(msg) qDebug() << "[DEBUG]" << msg
#else
#define DEBUG_LOG(msg)
#endif
```

#### 2. 运行时日志级别

```cpp
void setDebugLevel(int level) {
    if (level >= 1) {
        // 基本调试信息
    }
    if (level >= 2) {
        // 详细调试信息
    }
}
```

#### 3. 条件编译调试代码

```cpp
void ParameterHandler::setParameters(QSqlQuery& query, const QVariantMap& parameters)
{
#ifdef QT_MYBATIS_DEBUG
    qDebug() << "[ParameterHandler] SQL:" << query.lastQuery();
    qDebug() << "[ParameterHandler] Parameters:" << parameters;
#endif
    
    // 实际代码
}
```

### 预防措施

#### 1. 单元测试覆盖

确保每个组件都有独立的单元测试：

```cpp
// 测试参数绑定
void testParameterBinding();

// 测试SQL执行
void testSqlExecution();

// 测试事务管理
void testTransactionManagement();
```

#### 2. 集成测试

测试组件之间的交互：

```cpp
// 测试完整的CRUD操作
void testFullCRUDWorkflow();

// 测试连接池管理
void testConnectionPooling();
```

#### 3. 错误处理测试

专门测试错误情况：

```cpp
// 测试数据库连接失败
void testDatabaseConnectionFailure();

// 测试SQL语法错误
void testInvalidSQLHandling();
```

### 总结

这次调试过程的关键收获：

1. **系统性思考**：从错误信息开始，逐步深入到根本原因
2. **验证假设**：不要假设问题在某个特定地方，用测试验证
3. **隔离问题**：创建最小化的复现测试
4. **全面修复**：不仅修复表面问题，还要修复发现的相关问题
5. **文档记录**：记录问题和解决过程，为后续开发提供参考

通过这种系统性的调试方法，我们不仅解决了当前的问题，还发现并修复了框架中的其他潜在问题，提高了整体代码质量。 