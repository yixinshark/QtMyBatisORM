#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QDebug>

namespace QtMyBatisORM {

QtMyBatisORM::QtMyBatisORM(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

QtMyBatisORM::~QtMyBatisORM()
{
    shutdown();
}

bool QtMyBatisORM::initialize(const QString& configPath, const QStringList& mapperPaths)
{
    try {
        // 如果已经初始化，先关闭之前的资源
        if (m_initialized) {
            shutdown();
        }
        
        loadConfiguration(configPath);
        loadMappers(mapperPaths);
        
        m_sessionFactory = SessionFactory::create(m_config);
        m_initialized = true;
        
        return true;
    } catch (const QtMyBatisException& e) {
        qWarning("Failed to initialize QtMyBatisORM: %s", qPrintable(e.message()));
        return false;
    } catch (const std::exception& e) {
        qWarning("Failed to initialize QtMyBatisORM: %s", e.what());
        return false;
    } catch (...) {
        qWarning("Failed to initialize QtMyBatisORM: Unknown error");
        return false;
    }
}

bool QtMyBatisORM::initializeWithConfig(const DatabaseConfig& config, const QStringList& mapperPaths)
{
    try {
        // 如果已经初始化，先关闭之前的资源
        if (m_initialized) {
            shutdown();
        }
        
        m_config = config;
        loadMappers(mapperPaths);
        
        m_sessionFactory = SessionFactory::create(m_config);
        m_initialized = true;
        
        return true;
    } catch (const QtMyBatisException& e) {
        qWarning("Failed to initialize QtMyBatisORM with config: %s", qPrintable(e.message()));
        return false;
    } catch (const std::exception& e) {
        qWarning("Failed to initialize QtMyBatisORM with config: %s", e.what());
        return false;
    } catch (...) {
        qWarning("Failed to initialize QtMyBatisORM with config: Unknown error");
        return false;
    }
}

QSharedPointer<SessionFactory> QtMyBatisORM::getSessionFactory()
{
    if (!m_initialized) {
        throw ConfigurationException(QStringLiteral("QtMyBatisORM not initialized"));
    }
    
    return m_sessionFactory;
}

QSharedPointer<Session> QtMyBatisORM::openSession()
{
    if (!m_initialized) {
        throw ConfigurationException(QStringLiteral("QtMyBatisORM not initialized"));
    }
    
    return m_sessionFactory->openSession();
}

void QtMyBatisORM::closeSession(QSharedPointer<Session> session)
{
    if (m_sessionFactory && !session.isNull()) {
        m_sessionFactory->closeSession(session);
    }
}

bool QtMyBatisORM::isInitialized() const
{
    return m_initialized;
}

DatabaseConfig QtMyBatisORM::getDatabaseConfig() const
{
    return m_config;
}

void QtMyBatisORM::shutdown()
{
    if (m_initialized) {
        if (m_sessionFactory) {
            m_sessionFactory->close();
            m_sessionFactory.reset();
        }
        
        m_initialized = false;
    }
}

QSharedPointer<QtMyBatisORM> QtMyBatisORM::create(const QString& configPath, 
                                                 const QStringList& mapperPaths)
{
    QSharedPointer<QtMyBatisORM> instance = QSharedPointer<QtMyBatisORM>::create();
    
    if (instance->initialize(configPath, mapperPaths)) {
        return instance;
    }
    
    return nullptr;
}

QSharedPointer<QtMyBatisORM> QtMyBatisORM::createWithConfig(const DatabaseConfig& config,
                                                           const QStringList& mapperPaths)
{
    QSharedPointer<QtMyBatisORM> instance = QSharedPointer<QtMyBatisORM>::create();
    
    if (instance->initializeWithConfig(config, mapperPaths)) {
        return instance;
    }
    
    return nullptr;
}

void QtMyBatisORM::loadConfiguration(const QString& configPath)
{
    ConfigurationManager* configMgr = ConfigurationManager::instance();
    
    if (!configMgr->loadConfiguration(configPath)) {
        throw ConfigurationException(
            QStringLiteral("Failed to load configuration from: %1").arg(configPath)
        );
    }
    
    m_config = configMgr->getDatabaseConfig();
}

void QtMyBatisORM::loadMappers(const QStringList& mapperPaths)
{
    if (mapperPaths.isEmpty()) {
        return;
    }
    
    ConfigurationManager* configMgr = ConfigurationManager::instance();
    
    if (!configMgr->loadMappers(mapperPaths)) {
        throw ConfigurationException(QStringLiteral("Failed to load mapper configurations"));
    }
}

QSharedPointer<QtMyBatisORM> QtMyBatisORM::createDefault()
{
    DatabaseConfig config;
    config.driverName = QStringLiteral("QSQLITE");
    config.databaseName = QStringLiteral(":memory:");
    config.maxConnections = 10;
    config.minConnections = 2;
    config.maxIdleTime = 300;
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    
    return createWithConfig(config);
}

QSharedPointer<QtMyBatisORM> QtMyBatisORM::createSQLite(const QString& databasePath)
{
    DatabaseConfig config;
    config.driverName = QStringLiteral("QSQLITE");
    config.databaseName = databasePath;
    config.maxConnections = 10;
    config.minConnections = 2;
    config.maxIdleTime = 300;
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    
    return createWithConfig(config);
}

QSharedPointer<QtMyBatisORM> QtMyBatisORM::createMySQL(const QString& host, int port, 
                                                      const QString& database, 
                                                      const QString& username, 
                                                      const QString& password)
{
    DatabaseConfig config;
    config.driverName = QStringLiteral("QMYSQL");
    config.hostName = host;
    config.port = port;
    config.databaseName = database;
    config.userName = username;
    config.password = password;
    config.maxConnections = 10;
    config.minConnections = 2;
    config.maxIdleTime = 300;
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    
    return createWithConfig(config);
}

QSharedPointer<SessionFactory> QtMyBatisORM::createSessionFactory(const QString& configPath,
                                                                 const QStringList& mapperPaths)
{
    QSharedPointer<QtMyBatisORM> orm = create(configPath, mapperPaths);
    if (orm) {
        return orm->getSessionFactory();
    }
    return nullptr;
}

QSharedPointer<SessionFactory> QtMyBatisORM::createSessionFactoryWithConfig(const DatabaseConfig& config,
                                                                           const QStringList& mapperPaths)
{
    QSharedPointer<QtMyBatisORM> orm = createWithConfig(config, mapperPaths);
    if (orm) {
        return orm->getSessionFactory();
    }
    return nullptr;
}

} // namespace QtMyBatisORM