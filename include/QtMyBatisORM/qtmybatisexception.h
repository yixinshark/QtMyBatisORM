#pragma once

#include <QException>
#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include "export.h"

namespace QtMyBatisORM {

/**
 * Qt-MyBatis-ORM基础异常类
 */
class QTMYBATISORM_EXPORT QtMyBatisException : public QException
{
public:
    explicit QtMyBatisException(const QString& message, const QString& code = QLatin1String(""));
    explicit QtMyBatisException(const QString& message, const QString& code, const QString& detail);
    
    QString message() const;
    QString code() const;
    QString detail() const;
    QDateTime timestamp() const;
    
    // 添加上下文信息
    void setContext(const QVariantMap &context);
    void setContext(const QString& key, const QVariant& value);
    QVariant getContext(const QString& key) const;
    QVariantMap getAllContext() const;
    
    // 格式化完整错误信息
    QString fullMessage() const;
    
    // std::exception interface
    const char* what() const noexcept override;
    
    // QException interface
    void raise() const override;
    QtMyBatisException* clone() const override;
    
protected:
    QString m_message;
    QString m_code;
    QString m_detail;
    QDateTime m_timestamp;
    QVariantMap m_context;
    mutable QByteArray m_whatMessage; // Cache for what() method
};

/**
 * 配置异常
 */
class ConfigurationException : public QtMyBatisException
{
public:
    explicit ConfigurationException(const QString& message, const QString& code = QLatin1String("CONFIG_ERROR"));
    ConfigurationException* clone() const override;
};

/**
 * SQL执行异常
 */
class SqlExecutionException : public QtMyBatisException
{
public:
    explicit SqlExecutionException(const QString& message, const QString& code = QLatin1String("SQL_ERROR"));
    SqlExecutionException* clone() const override;
};

/**
 * 连接异常
 */
class ConnectionException : public QtMyBatisException
{
public:
    explicit ConnectionException(const QString& message, const QString& code = QLatin1String("CONNECTION_ERROR"));
    ConnectionException* clone() const override;
};

/**
 * 映射异常
 */
class MappingException : public QtMyBatisException
{
public:
    explicit MappingException(const QString& message, const QString& code = QLatin1String("MAPPING_ERROR"));
    MappingException* clone() const override;
};

/**
 * 缓存异常
 */
class CacheException : public QtMyBatisException
{
public:
    explicit CacheException(const QString& message, const QString& code = QLatin1String("CACHE_ERROR"));
    CacheException* clone() const override;
};

/**
 * 事务异常
 */
class TransactionException : public QtMyBatisException
{
public:
    explicit TransactionException(const QString& message, const QString& code = QLatin1String("TRANSACTION_ERROR"));
    TransactionException* clone() const override;
};

/**
 * 参数异常
 */
class ParameterException : public QtMyBatisException
{
public:
    explicit ParameterException(const QString& message, const QString& code = QLatin1String("PARAMETER_ERROR"));
    ParameterException* clone() const override;
};

/**
 * 结果处理异常
 */
class ResultException : public QtMyBatisException
{
public:
    explicit ResultException(const QString& message, const QString& code = QLatin1String("RESULT_ERROR"));
    ResultException* clone() const override;
};

/**
 * 会话异常
 */
class SessionException : public QtMyBatisException
{
public:
    explicit SessionException(const QString& message, const QString& code = QLatin1String("SESSION_ERROR"));
    SessionException* clone() const override;
};

} // namespace QtMyBatisORM