# Requirements Document

## Introduction

本项目旨在为Qt6开发一个类似于Java MyBatis的ORM（对象关系映射）组件。该组件将提供简洁的数据库操作接口，支持多种数据库，具备连接池管理功能，并通过XML配置文件和JSON配置来简化数据库操作。组件将采用现代C++标准和Qt6框架，使用CMake进行项目管理。

## Requirements

### Requirement 1

**User Story:** 作为一个使用Qt-MyBatis-ORM组件的开发者，我希望能够在我的Qt项目中通过资源文件提供JSON格式的数据库配置，这样组件可以读取我项目中的配置来建立数据库连接。

#### Acceptance Criteria

1. WHEN 组件初始化时 THEN 组件 SHALL 能够从调用方Qt项目的资源系统中读取指定路径的JSON格式数据库配置文件
2. WHEN 配置文件包含数据库类型、主机、端口、用户名、密码等信息时 THEN 组件 SHALL 正确解析并建立数据库连接
3. WHEN 配置文件格式错误时 THEN 组件 SHALL 抛出明确的错误信息，指出具体的格式问题
4. WHEN 调用方项目的Qt资源中不存在指定的配置文件时 THEN 组件 SHALL 抛出明确的错误提示，说明需要在qrc文件中添加配置文件
5. WHEN 组件作为共享库使用时 THEN 组件 SHALL 能够访问宿主应用程序的Qt资源系统
6. WHEN 组件作为源代码模块集成时 THEN 组件 SHALL 能够直接访问项目的Qt资源系统
7. WHEN 需要支持多环境配置时 THEN 组件 SHALL 支持通过初始化参数指定不同的资源文件路径（如":/config/database-dev.json"、":/config/database-prod.json"）

### Requirement 2

**User Story:** 作为一个开发者，我希望能够通过多个XML文件定义不同数据库表的SQL语句和映射关系，包括数据库表创建语句，这样我可以将SQL逻辑与C++代码分离，便于维护和管理。

#### Acceptance Criteria

1. WHEN 系统启动时 THEN 系统 SHALL 能够扫描并加载Qt资源系统中的多个XML映射文件
2. WHEN XML映射文件包含各种类型的SQL语句（SELECT、INSERT、UPDATE、DELETE、CREATE TABLE等）时 THEN 系统 SHALL 能够解析并执行这些语句
3. WHEN XML文件包含CREATE TABLE语句时 THEN 系统 SHALL 能够执行数据库表创建操作
4. WHEN XML文件定义了参数映射时 THEN 系统 SHALL 正确地将C++对象属性映射到SQL参数
5. WHEN XML文件定义了结果映射时 THEN 系统 SHALL 能够将查询结果映射回C++对象
6. WHEN XML文件包含动态SQL元素（如if、foreach等）时 THEN 系统 SHALL 根据运行时条件生成相应的SQL语句
7. WHEN XML文件包含DDL语句（CREATE、ALTER、DROP等）时 THEN 系统 SHALL 能够执行数据库结构变更操作
8. WHEN 多个XML文件中存在相同的SQL语句ID时 THEN 系统 SHALL 检测并报告冲突错误
9. WHEN XML文件语法错误时 THEN 系统 SHALL 在启动时检测并报告具体的文件和错误位置

### Requirement 3

**User Story:** 作为一个开发者，我希望能够定义DAO（数据访问对象）接口，通过简单的方法调用来执行数据库操作，而不需要编写重复的数据库连接和SQL执行代码。

#### Acceptance Criteria

1. WHEN 定义DAO接口时 THEN 系统 SHALL 能够通过反射或代码生成机制自动实现这些接口
2. WHEN DAO方法被调用时 THEN 系统 SHALL 自动查找对应的XML映射并执行SQL语句
3. WHEN DAO方法包含参数时 THEN 系统 SHALL 正确地将参数传递给SQL语句
4. WHEN DAO方法需要返回结果时 THEN 系统 SHALL 将数据库结果映射为相应的C++对象
5. WHEN DAO方法执行失败时 THEN 系统 SHALL 抛出包含详细错误信息的异常

### Requirement 4

**User Story:** 作为一个开发者，我希望组件能够支持MySQL兼容的数据库（如MySQL、SQLite等），这样我可以使用统一的API来操作这些数据库而不需要修改代码。

#### Acceptance Criteria

1. WHEN 配置数据库连接时 THEN 系统 SHALL 支持Qt的QMYSQL和QSQLITE驱动程序
2. WHEN 使用不同的MySQL兼容数据库时 THEN 系统 SHALL 正确处理标准SQL语法和数据类型
3. WHEN 数据库连接失败时 THEN 系统 SHALL 提供明确的错误信息并指明具体的数据库类型
4. WHEN 使用MySQL特有功能（如AUTO_INCREMENT、ON DUPLICATE KEY UPDATE等）时 THEN 系统 SHALL 在支持的数据库上正确执行，在不支持的数据库上提供明确的错误提示
5. WHEN 切换MySQL兼容的数据库类型时 THEN 现有的DAO接口和XML映射 SHALL 保持兼容

### Requirement 5

**User Story:** 作为一个开发者，我希望组件具备数据库连接池功能，这样我可以高效地管理数据库连接，提高应用程序的性能和稳定性。

#### Acceptance Criteria

1. WHEN 系统启动时 THEN 连接池 SHALL 根据配置创建指定数量的数据库连接
2. WHEN 需要执行数据库操作时 THEN 系统 SHALL 从连接池中获取可用连接
3. WHEN 数据库操作完成时 THEN 系统 SHALL 将连接返回到连接池中
4. WHEN 连接池中的连接超过最大空闲时间时 THEN 系统 SHALL 自动关闭多余的连接
5. WHEN 连接池中的连接发生错误时 THEN 系统 SHALL 自动重新创建连接
6. WHEN 连接池达到最大连接数时 THEN 系统 SHALL 根据配置等待或拒绝新的连接请求

### Requirement 6

**User Story:** 作为一个开发者，我希望能够使用CMake来管理项目构建，这样我可以轻松地在不同平台上编译和部署组件。

#### Acceptance Criteria

1. WHEN 使用CMake构建项目时 THEN 系统 SHALL 正确地链接Qt6库和数据库驱动
2. WHEN 在不同平台（Windows、Linux、macOS）上构建时 THEN CMake配置 SHALL 自动处理平台差异
3. WHEN 项目包含测试时 THEN CMake SHALL 能够构建和运行单元测试
4. WHEN 项目需要安装时 THEN CMake SHALL 提供正确的安装目标和头文件导出

### Requirement 7

**User Story:** 作为一个开发者，我希望组件提供事务管理功能，这样我可以确保数据库操作的一致性和完整性。

#### Acceptance Criteria

1. WHEN 开始事务时 THEN 系统 SHALL 在当前连接上启动数据库事务
2. WHEN 事务中的所有操作成功时 THEN 系统 SHALL 能够提交事务
3. WHEN 事务中任何操作失败时 THEN 系统 SHALL 能够回滚事务
4. WHEN 事务超时时 THEN 系统 SHALL 自动回滚事务并释放资源
5. WHEN 嵌套事务时 THEN 系统 SHALL 正确处理保存点机制

### Requirement 8

**User Story:** 作为一个开发者，我希望组件提供缓存机制，这样我可以提高频繁查询的性能。

#### Acceptance Criteria

1. WHEN 启用缓存时 THEN 系统 SHALL 缓存查询结果
2. WHEN 执行相同查询时 THEN 系统 SHALL 从缓存中返回结果而不是重新查询数据库
3. WHEN 数据发生变更操作（INSERT、UPDATE、DELETE）时 THEN 系统 SHALL 自动清除相关的缓存条目
4. WHEN 缓存达到最大容量时 THEN 系统 SHALL 使用LRU（最近最少使用）策略清除最少使用的缓存条目
5. WHEN 缓存条目超过配置的生存时间时 THEN 系统 SHALL 自动清除过期的缓存条目
6. WHEN 配置缓存参数时 THEN 系统 SHALL 支持设置最大缓存条目数量和缓存过期时间