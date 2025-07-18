# Mapper Integration Usage Guide

This document provides a comprehensive guide to using the mapper integration feature in the Qt-MyBatis-ORM library.

## 1. Overview

The mapper integration feature allows you to define interfaces for your database operations and have the ORM automatically implement them. This provides a type-safe way to interact with your database and makes your code more maintainable.

## 2. Defining Mapper Interfaces

Mapper interfaces are defined as Qt objects with virtual methods that correspond to SQL operations:

```cpp
class UserMapper : public QObject
{
    Q_OBJECT
    
public:
    virtual QVariantList findAll() = 0;
    virtual QVariant findById(int id) = 0;
    virtual int insert(const QVariantMap& user) = 0;
    virtual int update(const QVariantMap& user) = 0;
    virtual int deleteById(int id) = 0;
};
```

## 3. XML Mapper Configuration

The XML mapper configuration defines the SQL statements that will be executed for each method in the mapper interface:

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
        INSERT INTO users (name, email, created_at) 
        VALUES (:name, :email, :created_at)
    </insert>
    
    <update id="update" parameterType="User">
        UPDATE users SET name = :name, email = :email 
        WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM users WHERE id = :id
    </delete>
</mapper>
```

## 4. Using Mappers

### 4.1 Getting a Mapper Instance

To use a mapper, you first need to get an instance from the session:

```cpp
auto session = orm->openSession();
UserMapper* userMapper = session->getMapper<UserMapper>();
```

### 4.2 Using Mapper Methods

Once you have a mapper instance, you can call its methods to perform database operations:

```cpp
// Find all users
QVariantList users = userMapper->findAll();

// Find user by ID
QVariant user = userMapper->findById(1);

// Insert a new user
QVariantMap newUser;
newUser["name"] = "John Doe";
newUser["email"] = "john.doe@example.com";
newUser["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
int userId = userMapper->insert(newUser);

// Update a user
QVariantMap updateUser;
updateUser["id"] = userId;
updateUser["name"] = "Jane Doe";
updateUser["email"] = "jane.doe@example.com";
int updatedRows = userMapper->update(updateUser);

// Delete a user
int deletedRows = userMapper->deleteById(userId);
```

## 5. Advanced Mapper Features

### 5.1 Dynamic SQL

You can use dynamic SQL in your mapper methods to create flexible queries:

```xml
<select id="findByNameAndEmail" parameterType="map" resultType="User">
    SELECT * FROM users
    <where>
        <if test="name != null">
            name LIKE :name
        </if>
        <if test="email != null">
            AND email = :email
        </if>
    </where>
</select>
```

### 5.2 Result Mapping

You can map query results to custom types:

```xml
<resultMap id="userResultMap" type="User">
    <id property="id" column="user_id" />
    <result property="name" column="user_name" />
    <result property="email" column="user_email" />
    <result property="createdAt" column="created_at" />
</resultMap>

<select id="findAllWithMapping" resultMap="userResultMap">
    SELECT user_id, user_name, user_email, created_at 
    FROM users
</select>
```

### 5.3 One-to-Many Relationships

You can map one-to-many relationships:

```xml
<resultMap id="userWithOrdersMap" type="User">
    <id property="id" column="user_id" />
    <result property="name" column="user_name" />
    <result property="email" column="user_email" />
    <collection property="orders" ofType="Order">
        <id property="id" column="order_id" />
        <result property="amount" column="order_amount" />
        <result property="date" column="order_date" />
    </collection>
</resultMap>

<select id="findUserWithOrders" parameterType="int" resultMap="userWithOrdersMap">
    SELECT u.id as user_id, u.name as user_name, u.email as user_email,
           o.id as order_id, o.amount as order_amount, o.date as order_date
    FROM users u
    LEFT JOIN orders o ON u.id = o.user_id
    WHERE u.id = :id
</select>
```

## 6. Performance Optimizations

### 6.1 Mapper Proxy Caching

The mapper proxy implementation includes caching to improve performance:

```cpp
class MapperProxy : public QObject
{
    // ...
private:
    QHash<QString, QMetaMethod> m_methodCache;
    QHash<QString, QString> m_statementCache;
};
```

### 6.2 Statement Caching

SQL statements are cached to avoid repeated parsing:

```cpp
QString MapperProxy::getStatementId(const QString& methodName)
{
    // Check cache first
    if (m_statementCache.contains(methodName)) {
        return m_statementCache[methodName];
    }
    
    // Generate statement ID
    QString statementId = QString("%1.%2").arg(m_namespace, methodName);
    
    // Cache for future use
    m_statementCache[methodName] = statementId;
    
    return statementId;
}
```

### 6.3 Batch Operations

For better performance with bulk operations, you can use batch methods:

```cpp
// Batch insert
QList<QVariantMap> users;
// ... add users to the list
int insertedCount = session->batchInsert("UserMapper.insert", users);

// Batch update
QList<QVariantMap> updatedUsers;
// ... add users to update
int updatedCount = session->batchUpdate("UserMapper.update", updatedUsers);
```

## 7. Best Practices

### 7.1 Mapper Design

- Keep mapper interfaces focused on a single entity or related group of entities
- Use meaningful method names that reflect the operation being performed
- Use consistent parameter and result types

### 7.2 XML Mapper Organization

- Use namespaces that match your mapper interface names
- Use consistent ID naming conventions
- Group related operations together

### 7.3 Error Handling

- Handle exceptions properly when using mapper methods
- Use transactions for operations that modify multiple records

```cpp
try {
    session->beginTransaction();
    
    // Perform multiple operations
    userMapper->insert(user);
    addressMapper->insert(address);
    
    session->commit();
} catch (const QtMyBatisException& e) {
    session->rollback();
    qCritical() << "Error:" << e.message();
}
```

### 7.4 Resource Management

- Always close sessions when you're done with them
- Use RAII patterns to ensure proper resource cleanup

```cpp
{
    auto session = orm->openSession();
    // Use session...
    orm->closeSession(session);
}
```

## 8. Advanced Topics

### 8.1 Custom Type Handlers

You can register custom type handlers for complex types:

```cpp
// Register a custom type handler for QDateTime
TypeHandlerRegistry::registerTypeHandler<QDateTime>(
    [](const QDateTime& value) -> QVariant {
        return value.toString(Qt::ISODate);
    },
    [](const QVariant& value) -> QDateTime {
        return QDateTime::fromString(value.toString(), Qt::ISODate);
    }
);
```

### 8.2 Mapper Inheritance

You can use inheritance to create base mappers with common operations:

```cpp
class BaseMapper : public QObject
{
    Q_OBJECT
    
public:
    virtual int count() = 0;
    virtual QVariantList findAll() = 0;
    virtual QVariant findById(int id) = 0;
};

class UserMapper : public BaseMapper
{
    Q_OBJECT
    
public:
    // Inherited methods
    virtual int count() override;
    virtual QVariantList findAll() override;
    virtual QVariant findById(int id) override;
    
    // User-specific methods
    virtual QVariantList findByEmail(const QString& email) = 0;
};
```

### 8.3 Mapper Testing

You can create mock implementations of your mapper interfaces for testing:

```cpp
class MockUserMapper : public UserMapper
{
public:
    MOCK_METHOD0(findAll, QVariantList());
    MOCK_METHOD1(findById, QVariant(int));
    MOCK_METHOD1(insert, int(const QVariantMap&));
    MOCK_METHOD1(update, int(const QVariantMap&));
    MOCK_METHOD1(deleteById, int(int));
};
```

## 9. Troubleshooting

### 9.1 Common Issues

- **Method not found**: Ensure that the method name in the mapper interface matches the statement ID in the XML mapper.
- **Parameter type mismatch**: Ensure that the parameter types in the mapper interface match the parameter types in the XML mapper.
- **Result type mismatch**: Ensure that the result types in the mapper interface match the result types in the XML mapper.

### 9.2 Debugging

- Enable debug logging to see the SQL statements being executed
- Use the `printStats()` method on the session to see performance statistics

```cpp
Logger::setLogLevel(LogLevel::DEBUG);
// ... execute mapper methods
session->printStats();
```

## 10. Conclusion

The mapper integration feature in Qt-MyBatis-ORM provides a powerful way to interact with your database in a type-safe manner. By following the best practices outlined in this guide, you can create maintainable and efficient database access code.