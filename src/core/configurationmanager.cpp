#include "QtMyBatisORM/configurationmanager.h"
#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/xmlmapperparser.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QMutexLocker>
#include <QDebug>

namespace QtMyBatisORM {

ConfigurationManager* ConfigurationManager::s_instance = nullptr;
QMutex ConfigurationManager::s_mutex;

ConfigurationManager::ConfigurationManager(QObject* parent)
    : QObject(parent)
    , m_jsonParser(new JSONConfigParser(this))
    , m_xmlParser(new XMLMapperParser(this))
{
}

ConfigurationManager::~ConfigurationManager()
{
}

ConfigurationManager* ConfigurationManager::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new ConfigurationManager();
    }
    return s_instance;
}

bool ConfigurationManager::loadConfiguration(const QString& configPath)
{
    if (configPath.isEmpty()) {
        ConfigurationException ex(QLatin1String("Configuration path cannot be empty"), QLatin1String("CONFIG_EMPTY_PATH"));
        ex.setContext(QLatin1String("configPath"), configPath);
        throw ex;
    }
    
    QMutexLocker locker(&m_configMutex);
    
    try {
        m_dbConfig = m_jsonParser->parseConfiguration(configPath);
        qDebug() << "Configuration loaded successfully from:" << configPath;
        return true;
    } catch (const ConfigurationException& e) {
        // Add context and re-throw
        ConfigurationException ex(e);
        ex.setContext(QLatin1String("configPath"), configPath);
        ex.setContext(QLatin1String("operation"), QLatin1String("loadConfiguration"));
        throw ex;
    } catch (const QtMyBatisException& e) {
        // Wrap other exceptions as configuration exceptions
        ConfigurationException ex(
            QLatin1String("Failed to load configuration: ") + e.message(),
            QLatin1String("CONFIG_LOAD_ERROR")
        );
        ex.setContext(QLatin1String("configPath"), configPath);
        ex.setContext(QLatin1String("originalError"), e.message());
        ex.setContext(QLatin1String("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        ConfigurationException ex(
            QLatin1String("Unexpected error loading configuration: ") + QString::fromUtf8(e.what()),
            QLatin1String("CONFIG_UNEXPECTED_ERROR")
        );
        ex.setContext(QLatin1String("configPath"), configPath);
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

bool ConfigurationManager::loadMappers(const QStringList& mapperPaths)
{
    if (mapperPaths.isEmpty()) {
        ConfigurationException ex(QLatin1String("Mapper paths list cannot be empty"), QLatin1String("CONFIG_EMPTY_MAPPER_PATHS"));
        ex.setContext(QLatin1String("mapperPathsCount"), 0);
        throw ex;
    }
    
    QMutexLocker locker(&m_configMutex);
    
    try {
        QList<MapperConfig> mappers = m_xmlParser->parseMappers(mapperPaths);
        
        // 检查命名空间冲突
        for (const MapperConfig& mapper : mappers) {
            if (m_mappers.contains(mapper.namespace_)) {
                ConfigurationException ex(
                    QLatin1String("Duplicate mapper namespace: ") + mapper.namespace_,
                    QLatin1String("CONFIG_DUPLICATE_NAMESPACE")
                );
                ex.setContext(QLatin1String("namespace"), mapper.namespace_);
                ex.setContext(QLatin1String("xmlPath"), mapper.xmlPath);
                ex.setContext(QLatin1String("existingMapperPath"), m_mappers[mapper.namespace_].xmlPath);
                throw ex;
            }
            m_mappers[mapper.namespace_] = mapper;
        }
        
        qDebug() << "Successfully loaded" << mappers.size() << "mappers";
        return true;
    } catch (const ConfigurationException& e) {
        // Add context and re-throw
        ConfigurationException ex(e);
        ex.setContext(QLatin1String("mapperPaths"), mapperPaths);
        ex.setContext(QLatin1String("operation"), QLatin1String("loadMappers"));
        throw ex;
    } catch (const QtMyBatisException& e) {
        // Wrap other exceptions as configuration exceptions
        ConfigurationException ex(
            QLatin1String("Failed to load mappers: ") + e.message(),
            QLatin1String("CONFIG_MAPPER_LOAD_ERROR")
        );
        ex.setContext(QLatin1String("mapperPaths"), mapperPaths);
        ex.setContext(QLatin1String("originalError"), e.message());
        ex.setContext(QLatin1String("originalCode"), e.code());
        throw ex;
    } catch (const std::exception& e) {
        ConfigurationException ex(
            QLatin1String("Unexpected error loading mappers: ") + QString::fromUtf8(e.what()),
            QLatin1String("CONFIG_MAPPER_UNEXPECTED_ERROR")
        );
        ex.setContext(QLatin1String("mapperPaths"), mapperPaths);
        ex.setContext(QLatin1String("stdError"), QString::fromUtf8(e.what()));
        throw ex;
    }
}

DatabaseConfig ConfigurationManager::getDatabaseConfig() const
{
    QMutexLocker locker(&m_configMutex);
    return m_dbConfig;
}

QList<MapperConfig> ConfigurationManager::getMapperConfigs() const
{
    QMutexLocker locker(&m_configMutex);
    return m_mappers.values();
}

MapperConfig ConfigurationManager::getMapperConfig(const QString& namespace_) const
{
    QMutexLocker locker(&m_configMutex);
    return m_mappers.value(namespace_);
}

bool ConfigurationManager::hasMapper(const QString& namespace_) const
{
    QMutexLocker locker(&m_configMutex);
    return m_mappers.contains(namespace_);
}

void ConfigurationManager::reset()
{
    QMutexLocker locker(&m_configMutex);
    m_dbConfig = DatabaseConfig();
    m_mappers.clear();
}

bool ConfigurationManager::isConfigured() const
{
    QMutexLocker locker(&m_configMutex);
    return !m_dbConfig.driverName.isEmpty() && !m_dbConfig.databaseName.isEmpty();
}

int ConfigurationManager::getMapperCount() const
{
    QMutexLocker locker(&m_configMutex);
    return m_mappers.size();
}

QStringList ConfigurationManager::getMapperNamespaces() const
{
    QMutexLocker locker(&m_configMutex);
    return m_mappers.keys();
}

} // namespace QtMyBatisORM