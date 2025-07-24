#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QSharedPointer>
#include "datamodels.h"

namespace QtMyBatisORM {

class Session;

/**
 * Mapper registry
 * Mapper注册表
 */
class MapperRegistry : public QObject
{
    Q_OBJECT
    
public:
    explicit MapperRegistry(QObject* parent = nullptr);
    
    void registerMapper(const QString& mapperName, const MapperConfig& config);
    void registerMappers(const QList<MapperConfig>& configs);
    
    // Load Mappers from configuration manager
    bool loadMappersFromConfiguration();
    
    template<typename T>
    T* getMapper(QSharedPointer<Session> session);
    
    MapperConfig getMapperConfig(const QString& mapperName) const;
    bool hasMapper(const QString& mapperName) const;
    
    // Validation functionality
    bool validateMapper(const QString& mapperName) const;
    bool validateAllMappers() const;
    
    QStringList getMapperNames() const;
    QStringList getStatementIds(const QString& mapperName) const;
    int getMapperCount() const;
    
    void clear();
    
private:
    QObject* createMapperProxy(const QString& mapperName, QSharedPointer<Session> session);
    QString getMapperNameFromType(const QString& typeName);
    
    QHash<QString, MapperConfig> m_mappers;
    QHash<QString, QObject*> m_mapperInstances;
};

// Template method implementation
template<typename T>
T* MapperRegistry::getMapper(QSharedPointer<Session> session)
{
    // Get Mapper name from type name
    QString typeName = T::staticMetaObject.className();
    // Remove namespace prefix
    if (typeName.contains("::")) {
        typeName = typeName.split("::").last();
    }
    
    QString mapperName = getMapperNameFromType(typeName);
    
    // Create or get Mapper proxy instance
    // 创建或获取Mapper代理实例
    QString instanceKey = QString("%1_%2").arg(mapperName).arg(reinterpret_cast<quintptr>(session.data()));
    
    if (!m_mapperInstances.contains(instanceKey)) {
        QObject* proxy = createMapperProxy(mapperName, session);
        m_mapperInstances[instanceKey] = proxy;
    }
    
    return qobject_cast<T*>(m_mapperInstances[instanceKey]);
}

} // namespace QtMyBatisORM