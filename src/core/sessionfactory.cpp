#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/qtmybatisexception.h"

namespace QtMyBatisORM {

SessionFactory::SessionFactory(const DatabaseConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_closed(false)
{
    initialize();
}

QSharedPointer<SessionFactory> SessionFactory::create(const DatabaseConfig& config)
{
    return QSharedPointer<SessionFactory>(new SessionFactory(config));
}

void SessionFactory::initialize()
{
    m_connectionPool = QSharedPointer<ConnectionPool>::create(m_config);
    m_mapperRegistry = QSharedPointer<MapperRegistry>::create();
    m_cacheManager = QSharedPointer<CacheManager>::create(m_config);
    
    // 加载Mapper配置
    ConfigurationManager* configMgr = ConfigurationManager::instance();
    QList<MapperConfig> mappers = configMgr->getMapperConfigs();
    m_mapperRegistry->registerMappers(mappers);
}

QSharedPointer<Session> SessionFactory::openSession()
{
    if (m_closed) {
        throw ConfigurationException(QLatin1String("SessionFactory is closed"));
    }
    
    if (!m_connectionPool) {
        throw ConfigurationException(QLatin1String("Connection pool is not initialized"));
    }
    
    try {
        // 从连接池获取数据库连接
        QSharedPointer<QSqlDatabase> connection = m_connectionPool->getConnection();
        if (!connection) {
            throw ConnectionException(QLatin1String("Failed to get database connection from pool"));
        }
        
        // 创建Executor
        QSharedPointer<Executor> executor = QSharedPointer<Executor>::create(connection, m_cacheManager);
        
        // 创建Session
        QSharedPointer<Session> session = QSharedPointer<Session>::create(
            connection, executor, m_mapperRegistry
        );
        
        // 跟踪活动的Session
        m_activeSessions.insert(session.data());
        
        // 当Session被销毁时，从活动列表中移除并归还连接
        QObject::connect(session.data(), &QObject::destroyed, [this, connection](QObject* obj) {
            m_activeSessions.remove(obj);
            if (m_connectionPool && connection) {
                m_connectionPool->returnConnection(connection);
            }
        });
        
        return session;
        
    } catch (const QtMyBatisException&) {
        throw;
    } catch (const std::exception& e) {
        throw ConnectionException(QLatin1String("Failed to create session: ") + QString::fromUtf8(e.what()));
    }
}

void SessionFactory::closeSession(QSharedPointer<Session> session)
{
    if (session) {
        // 从活动Session列表中移除
        m_activeSessions.remove(session.data());
        
        // 关闭Session
        session->close();
    }
}

void SessionFactory::close()
{
    if (!m_closed) {
        // 关闭所有活动的Session
        QSet<QObject*> sessionsToClose = m_activeSessions;
        for (QObject* sessionObj : sessionsToClose) {
            Session* session = qobject_cast<Session*>(sessionObj);
            if (session) {
                session->close();
            }
        }
        m_activeSessions.clear();
        
        // 关闭连接池
        if (m_connectionPool) {
            m_connectionPool->close();
        }
        
        // 清理其他资源
        m_mapperRegistry.reset();
        m_cacheManager.reset();
        m_connectionPool.reset();
        
        m_closed = true;
    }
}

bool SessionFactory::isClosed() const
{
    return m_closed;
}

int SessionFactory::getActiveSessionCount() const
{
    return m_activeSessions.size();
}

} // namespace QtMyBatisORM