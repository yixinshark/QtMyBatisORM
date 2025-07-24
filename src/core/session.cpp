#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/logger.h"
#include <QTimer>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

namespace QtMyBatisORM {

Session::Session(QSharedPointer<QSqlDatabase> connection,
                QSharedPointer<Executor> executor,
                QSharedPointer<MapperRegistry> mapperRegistry,
                QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_executor(executor)
    , m_mapperRegistry(mapperRegistry)
    , m_autoCommit(true)
    , m_inTransaction(false)
    , m_closed(false)
    , m_transactionTimeoutSeconds(0)
    , m_transactionTimer(nullptr)
    , m_savepointCounter(0)
{
    m_transactionTimer = new QTimer(this);
    m_transactionTimer->setSingleShot(true);
    connect(m_transactionTimer, &QTimer::timeout, this, &Session::onTransactionTimeout);
}

QVariant Session::selectOne(const QString& statementId, const QVariantMap& parameters)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        return m_executor->queryWithCache(statementId, sql, parameters);
    } catch (const SessionException& e) {
        // Add context and re-throw
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("selectOne"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        // Wrap other exceptions as session exceptions
        SessionException ex(
            QStringLiteral("Failed to execute selectOne: %1").arg(e.message()),
            "SESSION_SELECT_ONE_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("selectOne");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in selectOne: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("selectOne");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

QVariantList Session::selectList(const QString& statementId, const QVariantMap& parameters)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        return m_executor->queryListWithCache(statementId, sql, parameters);
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("selectList"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute selectList: %1").arg(e.message()),
            "SESSION_SELECT_LIST_ERROR"
        );

        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("selectList");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in selectList: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("selectList");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

int Session::insert(const QString& statementId, const QVariantMap& parameters)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        return m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("insert"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute insert: %1").arg(e.message()),
            "SESSION_INSERT_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("insert");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in insert: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("insert");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

int Session::update(const QString& statementId, const QVariantMap& parameters)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        return m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("update"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute update: %1").arg(e.message()),
            "SESSION_UPDATE_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("update");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in update: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("update");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

int Session::remove(const QString& statementId, const QVariantMap& parameters)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        return m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("remove"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute remove: %1").arg(e.message()),
            "SESSION_REMOVE_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("remove");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in remove: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("remove");
        context[QStringLiteral("statementId")] = statementId;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

int Session::execute(const QString& sql, const QVariantMap& parameters)
{
    try {
        checkClosed();
        return m_executor->update(sql, parameters);
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("execute"));
        ex.setContext(QStringLiteral("sql"), sql);
        ex.setContext(QStringLiteral("parameters"), parameters);
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute SQL: %1").arg(e.message()),
            "SESSION_EXECUTE_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("execute");
        context[QStringLiteral("sql")] = sql;
        context[QStringLiteral("parameters")] = parameters;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in execute: %1") + QString::fromUtf8(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("execute");
        context[QStringLiteral("sql")] = sql;
        context[QStringLiteral("parameters")] = parameters;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

void Session::beginTransaction()
{
    beginTransaction(0); // 无超时
}

void Session::beginTransaction(int timeoutSeconds)
{
    try {
        checkClosed();
        checkTransactionTimeout();
        
        if (!m_connection || !m_connection->isOpen()) {
            TransactionException ex("Database connection is not available", "TRANSACTION_NO_CONNECTION");
            ex.setContext(QStringLiteral("connectionValid"), m_connection ? m_connection->isValid() : false);
            ex.setContext(QStringLiteral("connectionOpen"), m_connection ? m_connection->isOpen() : false);
            throw ex;
        }
        
        if (m_inTransaction) {
            TransactionException ex("Transaction is already active. Use savepoints for nested transactions.", "TRANSACTION_ALREADY_ACTIVE");
            ex.setContext(QStringLiteral("transactionLevel"), getTransactionLevel());
            ex.setContext(QStringLiteral("savepointCount"), m_savepointStack.size());
            throw ex;
        }
        
        if (!m_connection->transaction()) {
            TransactionException ex(
                QLatin1String("Failed to begin transaction: %1") +(m_connection->lastError().text()),
                "TRANSACTION_BEGIN_FAILED"
            );
            QVariantMap context;
            context[QStringLiteral("sqlError")] = m_connection->lastError().text();
            context[QStringLiteral("sqlErrorType")] = m_connection->lastError().type();
            context[QStringLiteral("timeoutSeconds")] = timeoutSeconds;
            ex.setContext(context);
            throw ex;
        }
        
        m_inTransaction = true;
        m_autoCommit = false;
        m_transactionStartTime = QDateTime::currentDateTime();
        m_transactionTimeoutSeconds = timeoutSeconds;
        
        // 设置事务超时定时器和预计算超时点
        if (timeoutSeconds > 0) {
            m_transactionTimeoutPoint = m_transactionStartTime.addSecs(timeoutSeconds);
            m_transactionTimer->start(timeoutSeconds * 1000);
        }
        
        // 调试日志
        if (isDebugMode()) {
            if (timeoutSeconds > 0) {
                qDebug() << QString("[Session] Begin transaction, timeout: %1 seconds").arg(timeoutSeconds);
            } else {
                qDebug() << "[Session] Begin transaction (no timeout limit)";
            }
        }
        
    } catch (const TransactionException& e) {
        TransactionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("beginTransaction"));
        ex.setContext(QStringLiteral("timeoutSeconds"), timeoutSeconds);
        throw ex;
    } catch (const QtMyBatisException& e) {
        TransactionException ex(
            QLatin1String("Failed to begin transaction: %1") +(e.message()),
            "TRANSACTION_BEGIN_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("beginTransaction");
        context[QStringLiteral("timeoutSeconds")] = timeoutSeconds;
        context[QStringLiteral("originalError")] = e.message();
        context[QStringLiteral("originalCode")] = e.code();
        ex.setContext(context);
        throw ex;
    } catch (const std::exception& e) {
        TransactionException ex(
            QLatin1String("Unexpected error beginning transaction: %1") +(e.what()),
            "TRANSACTION_UNEXPECTED_ERROR"
        );
        QVariantMap context;
        context[QLatin1String("operation")] = QLatin1String("beginTransaction");
        context[QStringLiteral("timeoutSeconds")] = timeoutSeconds;
        context[QLatin1String("stdError")] = QString::fromUtf8(e.what());
        ex.setContext(context);
        throw ex;
    }
}

void Session::commit()
{
    try {
        checkClosed();
        checkTransactionTimeout();
        
        if (!m_inTransaction) {
            TransactionException ex("No active transaction to commit", "TRANSACTION_NOT_ACTIVE");
            ex.setContext(QStringLiteral("transactionLevel"), getTransactionLevel());
            ex.setContext(QStringLiteral("autoCommit"), m_autoCommit);
            throw ex;
        }
        
        if (!m_connection || !m_connection->isOpen()) {
            TransactionException ex("Database connection is not available", "TRANSACTION_NO_CONNECTION");
            ex.setContext(QStringLiteral("connectionValid"), m_connection ? m_connection->isValid() : false);
            ex.setContext(QStringLiteral("connectionOpen"), m_connection ? m_connection->isOpen() : false);
            throw ex;
        }
        
        // 释放所有保存点
        QStringList failedSavepoints;
        while (!m_savepointStack.isEmpty()) {
            QString savepoint = m_savepointStack.pop();
            try {
                releaseSavepoint(savepoint);
            } catch (const QtMyBatisException& e) {
                // 记录失败的保存点但继续提交
                failedSavepoints.append(savepoint);
                qWarning() << "Failed to release savepoint" << savepoint << ":" << e.message();
            }
        }
        
        if (!m_connection->commit()) {
            TransactionException ex(
                QLatin1String("Failed to commit transaction: %1") +(m_connection->lastError().text()),
                "TRANSACTION_COMMIT_FAILED"
            );
            QVariantMap context;
            context[QStringLiteral("sqlError")] = m_connection->lastError().text();
            context[QStringLiteral("sqlErrorType")] = m_connection->lastError().type();
            context[QStringLiteral("failedSavepoints")] = failedSavepoints;
            context[QStringLiteral("transactionDuration")] = m_transactionStartTime.secsTo(QDateTime::currentDateTime());
            ex.setContext(context);
            throw ex;
        }
        
        // 调试日志
        if (isDebugMode()) {
            qint64 transactionDuration = m_transactionStartTime.secsTo(QDateTime::currentDateTime());
            qDebug() << QString("[Session] Transaction committed successfully, duration: %1 seconds").arg(transactionDuration);
        }
        
        // 清理事务状态
        m_inTransaction = false;
        m_autoCommit = true;
        m_transactionStartTime = QDateTime();
        m_transactionTimeoutSeconds = 0;
        m_transactionTimer->stop();
        
    } catch (const TransactionException& e) {
        TransactionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("commit"));
        throw ex;
    } catch (const QtMyBatisException& e) {
        TransactionException ex(
            QLatin1String("Failed to commit transaction: %1") +(e.message()),
            "TRANSACTION_COMMIT_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("commit"));
        ex.setContext(QStringLiteral("originalError"), e.message());
        ex.setContext(QStringLiteral("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        TransactionException ex(
            QLatin1String("Unexpected error committing transaction: %1") +(e.what()),
            "TRANSACTION_UNEXPECTED_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("commit"));
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

void Session::rollback()
{
    try {
        checkClosed();
        
        if (!m_inTransaction) {
            return; // 没有活动事务，静默返回
        }
        
        if (!m_connection || !m_connection->isOpen()) {
            TransactionException ex("Database connection is not available", "TRANSACTION_NO_CONNECTION");
            ex.setContext(QStringLiteral("connectionValid"), m_connection ? m_connection->isValid() : false);
            ex.setContext(QStringLiteral("connectionOpen"), m_connection ? m_connection->isOpen() : false);
            throw ex;
        }
        
        // 清理所有保存点
        m_savepointStack.clear();
        
        if (!m_connection->rollback()) {
            TransactionException ex(
                QLatin1String("Failed to rollback transaction: %1") +(m_connection->lastError().text()),
                "TRANSACTION_ROLLBACK_FAILED"
            );
            ex.setContext(QStringLiteral("sqlError"), m_connection->lastError().text());
            ex.setContext(QStringLiteral("sqlErrorType"), m_connection->lastError().type());
            ex.setContext(QStringLiteral("transactionDuration"), m_transactionStartTime.secsTo(QDateTime::currentDateTime()));
            throw ex;
        }
        
        // 调试日志
        if (isDebugMode()) {
            qint64 transactionDuration = m_transactionStartTime.secsTo(QDateTime::currentDateTime());
            qDebug() << QString("[Session] Transaction rolled back successfully, duration: %1 seconds").arg(transactionDuration);
        }
        
        // 清理事务状态
        m_inTransaction = false;
        m_autoCommit = true;
        m_transactionStartTime = QDateTime();
        m_transactionTimeoutSeconds = 0;
        m_transactionTimer->stop();
        
    } catch (const TransactionException& e) {
        TransactionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("rollback"));
        throw ex;
    } catch (const QtMyBatisException& e) {
        TransactionException ex(
            QLatin1String("Failed to rollback transaction: %1") +(e.message()),
            "TRANSACTION_ROLLBACK_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("rollback"));
        ex.setContext(QStringLiteral("originalError"), e.message());
        ex.setContext(QStringLiteral("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        TransactionException ex(
            QLatin1String("Unexpected error rolling back transaction: %1") +(e.what()),
            "TRANSACTION_UNEXPECTED_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("rollback"));
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

bool Session::isInTransaction() const
{
    return m_inTransaction;
}

void Session::close()
{
    if (!m_closed) {
        if (m_inTransaction) {
            rollback();
        }
        m_closed = true;
    }
}

bool Session::isClosed() const
{
    return m_closed;
}

void Session::setDebugMode(bool enabled)
{
    if (m_executor) {
        m_executor->setDebugMode(enabled);
    }
}

bool Session::isDebugMode() const
{
    if (m_executor) {
        return m_executor->isDebugMode();
    }
    return false;
}

QString Session::getStatementSql(const QString& statementId)
{
    // 使用静态缓存存储频繁访问的语句
    static QMutex cacheMutex;
    static QHash<QString, QString> sqlCache;
    
    // 首先检查缓存
    {
        QMutexLocker locker(&cacheMutex);
        if (sqlCache.contains(statementId)) {
            return sqlCache[statementId];
        }
    }
    
    if (!m_mapperRegistry) {
        throw MappingException(QLatin1String("MapperRegistry is not available"));
    }
    
    // 更高效的字符串解析
    int dotPos = statementId.indexOf(QLatin1Char('.'));
    if (dotPos <= 0 || dotPos == statementId.length() - 1) {
        throw MappingException(
            QStringLiteral("Invalid statement ID format: %1. Expected format: namespace.statementId")
            .arg(statementId)
        );
    }
    
    QString mapperName = statementId.left(dotPos);
    QString stmtId = statementId.mid(dotPos + 1);
    
    if (!m_mapperRegistry->hasMapper(mapperName)) {
        throw MappingException(
            QLatin1String("Mapper not found: %1") +(mapperName)
        );
    }
    
    MapperConfig config = m_mapperRegistry->getMapperConfig(mapperName);
    
    // 获取SQL语句
    QString result;
    QString fullStatementId = QStringLiteral("%1.%2").arg(mapperName, stmtId);
    
    if (config.statements.contains(fullStatementId)) {
        result = config.statements[fullStatementId].sql;
    } else if (config.statements.contains(stmtId)) {
        result = config.statements[stmtId].sql;
    } else {
        throw MappingException(
            QStringLiteral("Statement not found: %1 in mapper: %2")
            .arg(stmtId).arg(mapperName)
        );
    }
    
    // 缓存结果
    {
        QMutexLocker locker(&cacheMutex);
        if (sqlCache.size() > 1000) { // 防止无限增长
            sqlCache.clear();
        }
        sqlCache[statementId] = result;
    }
    
    return result;
}

// 嵌套事务支持 (保存点)
QString Session::setSavepoint(const QString& savepointName)
{
    checkClosed();
    checkTransactionTimeout();
    
    if (!m_inTransaction) {
        throw SqlExecutionException(QLatin1String("Cannot create savepoint outside of transaction"));
    }
    
    if (!m_connection || !m_connection->isOpen()) {
        throw SqlExecutionException(QLatin1String("Database connection is not available"));
    }
    
    QString actualSavepointName = savepointName.isEmpty() ? generateSavepointName() : savepointName;
    
    // 检查是否已经有这个保存点
    if (m_savepointStack.contains(actualSavepointName)) {
        return actualSavepointName; // 返回现有保存点，不创建新的
    }
    
    QSqlQuery query(*m_connection);
    QString sql = QLatin1String("SAVEPOINT %1") +(actualSavepointName);
    
    if (!query.exec(sql)) {
        throw SqlExecutionException(
            QStringLiteral("Failed to create savepoint '%1': %2")
            .arg(actualSavepointName)
            .arg(query.lastError().text())
        );
    }
    
    m_savepointStack.push(actualSavepointName);
    return actualSavepointName;
}

void Session::rollbackToSavepoint(const QString& savepointName)
{
    checkClosed();
    checkTransactionTimeout();
    
    if (!m_inTransaction) {
        throw SqlExecutionException(QLatin1String("Cannot rollback to savepoint outside of transaction"));
    }
    
    if (!m_connection || !m_connection->isOpen()) {
        throw SqlExecutionException(QLatin1String("Database connection is not available"));
    }
    
    if (!m_savepointStack.contains(savepointName)) {
        throw SqlExecutionException(
            QLatin1String("Savepoint '%1' not found") +(savepointName)
        );
    }
    
    QSqlQuery query(*m_connection);
    QString sql = QLatin1String("ROLLBACK TO SAVEPOINT %1") +(savepointName);
    
    if (!query.exec(sql)) {
        throw SqlExecutionException(
            QStringLiteral("Failed to rollback to savepoint '%1': %2")
            .arg(savepointName)
            .arg(query.lastError().text())
        );
    }
    
    // 移除该保存点之后的所有保存点
    while (!m_savepointStack.isEmpty() && m_savepointStack.top() != savepointName) {
        m_savepointStack.pop();
    }
}

void Session::releaseSavepoint(const QString& savepointName)
{
    checkClosed();
    checkTransactionTimeout();
    
    if (!m_inTransaction) {
        throw SqlExecutionException(QLatin1String("Cannot release savepoint outside of transaction"));
    }
    
    if (!m_connection || !m_connection->isOpen()) {
        throw SqlExecutionException(QLatin1String("Database connection is not available"));
    }
    
    if (!m_savepointStack.contains(savepointName)) {
        throw SqlExecutionException(
            QLatin1String("Savepoint '%1' not found") +(savepointName)
        );
    }
    
    QSqlQuery query(*m_connection);
    QString sql = QLatin1String("RELEASE SAVEPOINT %1") +(savepointName);
    
    if (!query.exec(sql)) {
        throw SqlExecutionException(
            QStringLiteral("Failed to release savepoint '%1': %2")
            .arg(savepointName)
            .arg(query.lastError().text())
        );
    }
    
    // 从栈中移除该保存点
    QStack<QString> tempStack;
    while (!m_savepointStack.isEmpty()) {
        QString sp = m_savepointStack.pop();
        if (sp != savepointName) {
            tempStack.push(sp);
        } else {
            break;
        }
    }
    
    // 恢复栈中剩余的保存点
    while (!tempStack.isEmpty()) {
        m_savepointStack.push(tempStack.pop());
    }
}

// 事务状态查询
int Session::getTransactionLevel() const
{
    return m_savepointStack.size() + (m_inTransaction ? 1 : 0);
}

QDateTime Session::getTransactionStartTime() const
{
    return m_transactionStartTime;
}

bool Session::isTransactionTimedOut() const
{
    if (!m_inTransaction || m_transactionTimeoutSeconds <= 0) {
        return false;
    }
    
    // 使用预先计算的超时点进行比较，避免重复计算
    return QDateTime::currentDateTime() >= m_transactionTimeoutPoint;
}

// 私有方法实现
void Session::onTransactionTimeout()
{
    if (m_inTransaction) {
        try {
            Logger::warn(QLatin1String("Transaction timed out and will be automatically rolled back"), {
                {QLatin1String("transactionStartTime"), m_transactionStartTime.toString(Qt::ISODate)},
                {QLatin1String("timeoutSeconds"), m_transactionTimeoutSeconds},
                {QLatin1String("elapsedSeconds"), m_transactionStartTime.secsTo(QDateTime::currentDateTime())}
            });
            
            rollback();
            
            Logger::info(QLatin1String("Transaction successfully rolled back after timeout"));
        } catch (const QtMyBatisException& e) {
            // 记录错误但不抛出异常，因为这是在定时器回调中
            Logger::error(QLatin1String("Failed to rollback timed-out transaction"), {
                {QLatin1String("errorMessage"), e.message()},
                {QLatin1String("errorCode"), e.code()},
                {QLatin1String("transactionStartTime"), m_transactionStartTime.toString(Qt::ISODate)}
            });
        }
        
        // 不在定时器回调中抛出异常，而是设置一个标志
        // 异常将在下次调用 checkTransactionTimeout() 时抛出
    }
}

void Session::checkTransactionTimeout()
{
    // 只在启用事务超时时检查
    if (m_inTransaction && m_transactionTimeoutSeconds > 0) {
        // 检查是否已经超过缓存的超时点
        QDateTime now = QDateTime::currentDateTime();
        if (now >= m_transactionTimeoutPoint) {
            rollback();
            throw SqlExecutionException(QLatin1String("Transaction has timed out"));
        }
    }
}

QString Session::generateSavepointName()
{
    return QStringLiteral("sp_%1").arg(++m_savepointCounter);
}

void Session::checkClosed()
{
    if (m_closed) {
        throw SqlExecutionException(QLatin1String("Session is closed"));
    }
}

int Session::batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        
        // 开始事务进行批量操作
        bool wasInTransaction = m_inTransaction;
        if (!wasInTransaction) {
            beginTransaction();
        }
        
        int totalAffected = 0;
        try {
            for (const QVariantMap& parameters : parametersList) {
                totalAffected += m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
            }
            
            // 如果是我们开始的事务，则提交
            if (!wasInTransaction) {
                commit();
            }
        } catch (...) {
            // 如果是我们开始的事务，则回滚
            if (!wasInTransaction) {
                rollback();
            }
            throw;
        }
        
        return totalAffected;
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("batchInsert"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute batch insert: %1").arg(e.message()),
            "SESSION_BATCH_INSERT_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchInsert"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QStringLiteral("originalError"), e.message());
        ex.setContext(QStringLiteral("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in batch insert: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchInsert"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

int Session::batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        
        // 开始事务进行批量操作
        bool wasInTransaction = m_inTransaction;
        if (!wasInTransaction) {
            beginTransaction();
        }
        
        int totalAffected = 0;
        try {
            for (const QVariantMap& parameters : parametersList) {
                totalAffected += m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
            }
            
            // 如果是我们开始的事务，则提交
            if (!wasInTransaction) {
                commit();
            }
        } catch (...) {
            // 如果是我们开始的事务，则回滚
            if (!wasInTransaction) {
                rollback();
            }
            throw;
        }
        
        return totalAffected;
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("batchUpdate"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute batch update: %1").arg(e.message()),
            "SESSION_BATCH_UPDATE_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchUpdate"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QStringLiteral("originalError"), e.message());
        ex.setContext(QStringLiteral("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in batch update: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchUpdate"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

int Session::batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    try {
        checkClosed();
        QString sql = getStatementSql(statementId);
        
        // 开始事务进行批量操作
        bool wasInTransaction = m_inTransaction;
        if (!wasInTransaction) {
            beginTransaction();
        }
        
        int totalAffected = 0;
        try {
            for (const QVariantMap& parameters : parametersList) {
                totalAffected += m_executor->updateWithCacheInvalidation(statementId, sql, parameters);
            }
            
            // 如果是我们开始的事务，则提交
            if (!wasInTransaction) {
                commit();
            }
        } catch (...) {
            // 如果是我们开始的事务，则回滚
            if (!wasInTransaction) {
                rollback();
            }
            throw;
        }
        
        return totalAffected;
    } catch (const SessionException& e) {
        SessionException ex(e);
        ex.setContext(QLatin1String("operation"), QLatin1String("batchRemove"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        throw ex;
    } catch (const QtMyBatisException& e) {
        SessionException ex(
            QStringLiteral("Failed to execute batch remove: %1").arg(e.message()),
            "SESSION_BATCH_REMOVE_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchRemove"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QStringLiteral("originalError"), e.message());
        ex.setContext(QStringLiteral("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        SessionException ex(
            QLatin1String("Unexpected error in batch remove: %1") +(e.what()),
            "SESSION_UNEXPECTED_ERROR"
        );
        ex.setContext(QLatin1String("operation"), QLatin1String("batchRemove"));
        ex.setContext(QStringLiteral("statementId"), statementId);
        ex.setContext(QStringLiteral("batchSize"), parametersList.size());
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

} // namespace QtMyBatisORM