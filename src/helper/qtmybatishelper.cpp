#include "QtMyBatisORM/qtmybatishelper.h"
#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/qtmybatisexception.h"

#include <QDebug>
#include <QElapsedTimer>

namespace QtMyBatisORM {

// RAII Session Manager - Ensures Session is always properly closed
// RAII Session管理器 - 确保Session总是被正确关闭
class QtMyBatisHelper::SessionScope
{
public:
    SessionScope()
    {
        if (!QtMyBatisHelper::s_orm) {
            throw ConfigurationException("QtMyBatisHelper not initialized");
        }
        
        m_session = QtMyBatisHelper::s_orm->openSession();
        if (!m_session) {
            throw ConnectionException("Failed to open session");
        }
        
        // 将调试模式传递给Session
        m_session->setDebugMode(QtMyBatisHelper::s_debugMode);
        if (QtMyBatisHelper::s_debugMode) {
            m_timer.start();
        }
    }
    
    ~SessionScope() 
    {
        if (!m_session) {
            return;
        }
        try {
            if (QtMyBatisHelper::s_debugMode) {
                qDebug() << "[QtMyBatisHelper] current operate spend time:" << m_timer.elapsed();
            }
            // 确保回滚任何未提交的事务
            if (m_session->isInTransaction()) {
                m_session->rollback();
                if (QtMyBatisHelper::s_debugMode) {
                    qDebug() << "[QtMyBatisHelper] Auto rollback uncommitted transaction";
                }
            }
            
            // Explicitly close Session and return connection
            // 显式关闭Session并归还连接
            QtMyBatisHelper::s_orm->closeSession(m_session);
        } catch (...) {
            // Do not throw exceptions in destructor
            qWarning() << "[QtMyBatisHelper] Exception occurred while closing session";
        }
    }
    
    QSharedPointer<Session> operator->() { return m_session; }
    QSharedPointer<Session> get() { return m_session; }
    
private:
    QSharedPointer<Session> m_session;
    QElapsedTimer m_timer;
};

QSharedPointer<QtMyBatisORM> QtMyBatisHelper::s_orm = nullptr;
bool QtMyBatisHelper::s_debugMode = false;
bool QtMyBatisHelper::s_initialized = false;

bool QtMyBatisHelper::initialize(const QString& configResourcePath)
{
    try {
        s_orm = QtMyBatisORM::createFromResource(configResourcePath);
        if (!s_orm) {
            qCritical() << "Failed to initialize QtMyBatisORM from:" << configResourcePath;
            return false;
        }
        
        s_initialized = true;
        qDebug() << "QtMyBatisHelper initialized successfully";
        return true;
    } catch (const QtMyBatisException& e) {
        qCritical() << "QtMyBatisHelper initialization failed:" << e.message();
        return false;
    }
}

void QtMyBatisHelper::shutdown()
{
    if (s_orm) {
        s_orm->shutdown();
        s_orm.reset();
    }
    s_initialized = false;
    qDebug() << "QtMyBatisHelper shutdown";
}

bool QtMyBatisHelper::isInitialized()
{
    return s_initialized && !s_orm.isNull();
}

QVariant QtMyBatisHelper::selectOne(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();

    // 使用RAII确保Session正确关闭
    SessionScope session;
    return session->selectOne(statementId, parameters);
}

QVariantList QtMyBatisHelper::selectList(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();

    SessionScope session;
    return session->selectList(statementId, parameters);
}

int QtMyBatisHelper::insert(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();

    SessionScope session;
    return session->insert(statementId, parameters);
}

int QtMyBatisHelper::update(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();

    SessionScope session;
    return session->update(statementId, parameters);
}

int QtMyBatisHelper::remove(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();
    
    SessionScope session;
    return session->remove(statementId, parameters);
}

int QtMyBatisHelper::execute(const QString& sql, const QVariantMap& parameters)
{
    checkInitialized();

    SessionScope session;
    
    return session->execute(sql, parameters);
}

int QtMyBatisHelper::batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();
    
    SessionScope session;
    return session->batchInsert(statementId, parametersList);
}

int QtMyBatisHelper::batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();
    
    SessionScope session;
    
    return session->batchUpdate(statementId, parametersList);
}

int QtMyBatisHelper::batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();

    SessionScope session;
    return session->batchRemove(statementId, parametersList);
}

bool QtMyBatisHelper::executeInTransaction(std::function<bool()> operation)
{
    checkInitialized();
    
    SessionScope session;
    
    try {
        session->beginTransaction();
        
        if (s_debugMode) {
            qDebug() << "[QtMyBatisHelper] Begin transaction";
        }
        
        bool success = operation();
        
        if (success) {
            session->commit();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction committed successfully";
            }
        } else {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction rolled back (operation returned false)";
            }
        }
        
        return success;
        
    } catch (...) {
        try {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction rolled back (exception occurred)";
            }
        } catch (...) {
            // Rollback failed, log but do not throw exception
            qWarning() << "[QtMyBatisHelper] Transaction rollback failed";
        }
        throw;
    }
}

bool QtMyBatisHelper::executeInTransaction(std::function<bool(QSharedPointer<Session>)> operation)
{
    checkInitialized();
    
    SessionScope session;
    
    try {
        session->beginTransaction();
        
        if (s_debugMode) {
            qDebug() << "[QtMyBatisHelper] Begin transaction";
        }
        
        bool success = operation(session.get());
        
        if (success) {
            session->commit();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction committed successfully";
            }
        } else {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction rolled back (operation returned false)";
            }
        }
        
        return success;
        
    } catch (...) {
        try {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] Transaction rolled back (exception occurred)";
            }
        } catch (...) {
            qWarning() << "[QtMyBatisHelper] Transaction rollback failed";
        }
        throw;
    }
}

void QtMyBatisHelper::setDebugMode(bool enabled)
{
    s_debugMode = enabled;
}

bool QtMyBatisHelper::isDebugMode()
{
    return s_debugMode;
}

void QtMyBatisHelper::checkInitialized()
{
    if (!s_initialized || s_orm.isNull()) {
        throw ConfigurationException("QtMyBatisHelper not initialized. Call initialize() first.");
    }
}

} // namespace QtMyBatisORM 