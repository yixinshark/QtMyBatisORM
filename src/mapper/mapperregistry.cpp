#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/mapperproxy.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/configurationmanager.h"
#include <QDebug>

namespace QtMyBatisORM {

MapperRegistry::MapperRegistry(QObject* parent)
    : QObject(parent)
{
}

void MapperRegistry::registerMapper(const QString& mapperName, const MapperConfig& config)
{
    if (m_mappers.contains(mapperName)) {
        throw MappingException(
            QStringLiteral("Mapper already registered: %1").arg(mapperName)
        );
    }
    
    m_mappers[mapperName] = config;
}

void MapperRegistry::registerMappers(const QList<MapperConfig>& configs)
{
    for (const MapperConfig& config : configs) {
        registerMapper(config.namespace_, config);
    }
}

bool MapperRegistry::loadMappersFromConfiguration()
{
    try {
        ConfigurationManager* configManager = ConfigurationManager::instance();
        QList<MapperConfig> configs = configManager->getMapperConfigs();
        
        if (configs.isEmpty()) {
            qWarning() << "No mapper configurations found";
            return false;
        }
        
        registerMappers(configs);
        qDebug() << "Successfully loaded" << configs.size() << "mappers from configuration";
        return true;
    } catch (const QtMyBatisException& e) {
        qWarning() << "Failed to load mappers from configuration:" << e.message();
        return false;
    }
}

bool MapperRegistry::validateMapper(const QString& mapperName) const
{
    if (!hasMapper(mapperName)) {
        return false;
    }
    
    MapperConfig config = getMapperConfig(mapperName);
    
    // 验证基本配置
    if (config.namespace_.isEmpty()) {
        qWarning() << "Mapper" << mapperName << "has empty namespace";
        return false;
    }
    
    if (config.statements.isEmpty()) {
        qWarning() << "Mapper" << mapperName << "has no statements";
        return false;
    }
    
    // 验证每个语句配置
    for (auto it = config.statements.begin(); it != config.statements.end(); ++it) {
        const StatementConfig& stmt = it.value();
        if (stmt.id.isEmpty() || stmt.sql.isEmpty()) {
            qWarning() << "Invalid statement in mapper" << mapperName << ":" << stmt.id;
            return false;
        }
    }
    
    return true;
}

bool MapperRegistry::validateAllMappers() const
{
    bool allValid = true;
    for (const QString& mapperName : getMapperNames()) {
        if (!validateMapper(mapperName)) {
            allValid = false;
        }
    }
    return allValid;
}

int MapperRegistry::getMapperCount() const
{
    return m_mappers.size();
}

QStringList MapperRegistry::getStatementIds(const QString& mapperName) const
{
    if (!hasMapper(mapperName)) {
        return QStringList();
    }
    
    MapperConfig config = getMapperConfig(mapperName);
    return config.statements.keys();
}

MapperConfig MapperRegistry::getMapperConfig(const QString& mapperName) const
{
    if (!m_mappers.contains(mapperName)) {
        throw MappingException(
            QStringLiteral("Mapper not found: %1").arg(mapperName)
        );
    }
    
    return m_mappers[mapperName];
}

bool MapperRegistry::hasMapper(const QString& mapperName) const
{
    return m_mappers.contains(mapperName);
}

QStringList MapperRegistry::getMapperNames() const
{
    return m_mappers.keys();
}

void MapperRegistry::clear()
{
    m_mappers.clear();
    
    // 清理Mapper实例
    for (auto instance : m_mapperInstances) {
        if (instance) {
            instance->deleteLater();
        }
    }
    m_mapperInstances.clear();
}

QObject* MapperRegistry::createMapperProxy(const QString& mapperName, QSharedPointer<Session> session)
{
    if (!hasMapper(mapperName)) {
        throw MappingException(
            QStringLiteral("Mapper not registered: %1").arg(mapperName)
        );
    }
    
    MapperConfig config = getMapperConfig(mapperName);
    return new MapperProxy(mapperName, session, config);
}

QString MapperRegistry::getMapperNameFromType(const QString& typeName)
{
    // 从类型名称推导Mapper名称
    // 例如：UserMapper -> UserMapper
    // 或者：User -> UserMapper
    if (typeName.endsWith(QStringLiteral("Mapper"))) {
        return typeName;
    } else {
        return typeName + "Mapper";
    }
}

} // namespace QtMyBatisORM