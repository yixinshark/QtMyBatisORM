#include "QtMyBatisORM/qtmybatishelper.h"
#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QDebug>
#include <QElapsedTimer>

namespace QtMyBatisORM {

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
    }
    
    ~SessionScope() 
    {
        if (m_session) {
            try {
                // 确保回滚任何未提交的事务
                if (m_session->isInTransaction()) {
                    m_session->rollback();
                    if (QtMyBatisHelper::s_debugMode) {
                        qDebug() << "[QtMyBatisHelper] 自动回滚未提交的事务";
                    }
                }
                
                // 显式关闭Session并归还连接
                QtMyBatisHelper::s_orm->closeSession(m_session);
                
                if (QtMyBatisHelper::s_debugMode) {
                    qDebug() << "[QtMyBatisHelper] Session已关闭，连接已归还";
                }
            } catch (...) {
                // 析构函数中不抛出异常
                qWarning() << "[QtMyBatisHelper] Session关闭时发生异常";
            }
        }
    }
    
    QSharedPointer<Session> operator->() { return m_session; }
    QSharedPointer<Session> get() { return m_session; }
    
private:
    QSharedPointer<Session> m_session;
    

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
    
    QElapsedTimer timer;
    timer.start();
    
    // 使用RAII确保Session正确关闭
    SessionScope session;
    
    QVariant result = session->selectOne(statementId, parameters);
    
    if (s_debugMode) {
        logDebug("selectOne", statementId, parameters, timer.elapsed(), result);
    }
    
    return result;
    // session在此处自动析构，Session被正确关闭
}

QVariantList QtMyBatisHelper::selectList(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    QVariantList result = session->selectList(statementId, parameters);
    
    if (s_debugMode) {
        logDebug("selectList", statementId, parameters, timer.elapsed(), 
                QString("返回%1条记录").arg(result.size()));
    }
    
    return result;
}

int QtMyBatisHelper::insert(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->insert(statementId, parameters);
    
    if (s_debugMode) {
        logDebug("insert", statementId, parameters, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::update(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->update(statementId, parameters);
    
    if (s_debugMode) {
        logDebug("update", statementId, parameters, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::remove(const QString& statementId, const QVariantMap& parameters)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->remove(statementId, parameters);
    
    if (s_debugMode) {
        logDebug("remove", statementId, parameters, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::execute(const QString& sql, const QVariantMap& parameters)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->execute(sql, parameters);
    
    if (s_debugMode) {
        logDebug("execute", sql, parameters, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->batchInsert(statementId, parametersList);
    
    if (s_debugMode) {
        QVariantMap summaryParams;
        summaryParams["batchSize"] = parametersList.size();
        logDebug("batchInsert", statementId, summaryParams, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->batchUpdate(statementId, parametersList);
    
    if (s_debugMode) {
        QVariantMap summaryParams;
        summaryParams["batchSize"] = parametersList.size();
        logDebug("batchUpdate", statementId, summaryParams, timer.elapsed(), result);
    }
    
    return result;
}

int QtMyBatisHelper::batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList)
{
    checkInitialized();
    
    QElapsedTimer timer;
    timer.start();
    
    SessionScope session;
    
    int result = session->batchRemove(statementId, parametersList);
    
    if (s_debugMode) {
        QVariantMap summaryParams;
        summaryParams["batchSize"] = parametersList.size();
        logDebug("batchRemove", statementId, summaryParams, timer.elapsed(), result);
    }
    
    return result;
}

bool QtMyBatisHelper::executeInTransaction(std::function<bool()> operation)
{
    checkInitialized();
    
    SessionScope session;
    
    try {
        session->beginTransaction();
        
        if (s_debugMode) {
            qDebug() << "[QtMyBatisHelper] 开始事务";
        }
        
        bool success = operation();
        
        if (success) {
            session->commit();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务提交成功";
            }
        } else {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务回滚（操作返回false）";
            }
        }
        
        return success;
        
    } catch (...) {
        try {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务回滚（发生异常）";
            }
        } catch (...) {
            // 回滚失败，记录日志但不再抛出异常
            qWarning() << "[QtMyBatisHelper] 事务回滚失败";
        }
        throw;
    }
    // session在此处自动析构，确保连接归还
}

bool QtMyBatisHelper::executeInTransaction(std::function<bool(QSharedPointer<Session>)> operation)
{
    checkInitialized();
    
    SessionScope session;
    
    try {
        session->beginTransaction();
        
        if (s_debugMode) {
            qDebug() << "[QtMyBatisHelper] 开始事务";
        }
        
        bool success = operation(session.get());
        
        if (success) {
            session->commit();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务提交成功";
            }
        } else {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务回滚（操作返回false）";
            }
        }
        
        return success;
        
    } catch (...) {
        try {
            session->rollback();
            if (s_debugMode) {
                qDebug() << "[QtMyBatisHelper] 事务回滚（发生异常）";
            }
        } catch (...) {
            qWarning() << "[QtMyBatisHelper] 事务回滚失败";
        }
        throw;
    }
    // session在此处自动析构，确保连接归还
}

void QtMyBatisHelper::enableDebugMode(bool enabled)
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

void QtMyBatisHelper::logDebug(const QString& operation, const QString& statementId, 
                              const QVariantMap& parameters, qint64 elapsedMs, 
                              const QVariant& result)
{
    if (!s_debugMode) return;
    
    QString paramStr;
    if (!parameters.isEmpty()) {
        QStringList paramList;
        for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
            paramList << QString("%1=%2").arg(it.key()).arg(it.value().toString());
        }
        paramStr = QString(" 参数:[%1]").arg(paramList.join(", "));
    }
    
    QString resultStr;
    if (result.isValid()) {
        if (result.canConvert<QVariantList>()) {
            resultStr = QString(" 结果:[返回%1条记录]").arg(result.toList().size());
        } else if (result.canConvert<QVariantMap>()) {
            QVariantMap map = result.toMap();
            resultStr = QString(" 结果:[对象包含%1个字段]").arg(map.size());
        } else {
            resultStr = QString(" 结果:[%1]").arg(result.toString());
        }
    }
    
    qDebug() << QString("[QtMyBatisHelper DEBUG] %1: %2%3%4 耗时:%5ms [Session已自动关闭]")
                .arg(operation)
                .arg(statementId)
                .arg(paramStr)
                .arg(resultStr)
                .arg(elapsedMs);
}

} // namespace QtMyBatisORM 