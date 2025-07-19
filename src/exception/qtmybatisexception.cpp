#include "QtMyBatisORM/qtmybatisexception.h"

namespace QtMyBatisORM {

QtMyBatisException::QtMyBatisException(const QString& message, const QString& code)
    : m_message(message), m_code(code), m_timestamp(QDateTime::currentDateTime())
{
}

QtMyBatisException::QtMyBatisException(const QString& message, const QString& code, const QString& detail)
    : m_message(message), m_code(code), m_detail(detail), m_timestamp(QDateTime::currentDateTime())
{
}

QString QtMyBatisException::message() const
{
    return m_message;
}

QString QtMyBatisException::code() const
{
    return m_code;
}

QString QtMyBatisException::detail() const
{
    return m_detail;
}

QDateTime QtMyBatisException::timestamp() const
{
    return m_timestamp;
}

void QtMyBatisException::setContext(const QVariantMap &context)
{
    m_context = context;
}

void QtMyBatisException::setContext(const QString& key, const QVariant& value)
{
    m_context[key] = value;
}

QVariant QtMyBatisException::getContext(const QString& key) const
{
    return m_context.value(key);
}

QVariantMap QtMyBatisException::getAllContext() const
{
    return m_context;
}

QString QtMyBatisException::fullMessage() const
{
    QString full = QStringLiteral("[%1] %2").arg(m_code, m_message);
    if (!m_detail.isEmpty()) {
        full += QStringLiteral(" - %1").arg(m_detail);
    }
    if (!m_context.isEmpty()) {
        full += QStringLiteral(" (Context: ");
        QStringList contextItems;
        contextItems.reserve(m_context.size()); // 预分配空间
        
        for (auto it = m_context.begin(); it != m_context.end(); ++it) {
            contextItems << QStringLiteral("%1=%2").arg(it.key(), it.value().toString());
        }
        full += contextItems.join(QStringLiteral(", ")) + QStringLiteral(")");
    }
    return full;
}

const char* QtMyBatisException::what() const noexcept
{
    if (m_whatMessage.isEmpty()) {
        m_whatMessage = m_message.toUtf8();
    }
    return m_whatMessage.constData();
}

void QtMyBatisException::raise() const
{
    throw *this;
}

QtMyBatisException* QtMyBatisException::clone() const
{
    return new QtMyBatisException(*this);
}

// ConfigurationException
ConfigurationException::ConfigurationException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

ConfigurationException* ConfigurationException::clone() const
{
    return new ConfigurationException(*this);
}

// SqlExecutionException
SqlExecutionException::SqlExecutionException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

SqlExecutionException* SqlExecutionException::clone() const
{
    return new SqlExecutionException(*this);
}

// ConnectionException
ConnectionException::ConnectionException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

ConnectionException* ConnectionException::clone() const
{
    return new ConnectionException(*this);
}

// MappingException
MappingException::MappingException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

MappingException* MappingException::clone() const
{
    return new MappingException(*this);
}

// CacheException
CacheException::CacheException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

CacheException* CacheException::clone() const
{
    return new CacheException(*this);
}

// TransactionException
TransactionException::TransactionException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

TransactionException* TransactionException::clone() const
{
    return new TransactionException(*this);
}

// ParameterException
ParameterException::ParameterException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

ParameterException* ParameterException::clone() const
{
    return new ParameterException(*this);
}

// ResultException
ResultException::ResultException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

ResultException* ResultException::clone() const
{
    return new ResultException(*this);
}

// SessionException
SessionException::SessionException(const QString& message, const QString& code)
    : QtMyBatisException(message, code)
{
}

SessionException* SessionException::clone() const
{
    return new SessionException(*this);
}

} // namespace QtMyBatisORM