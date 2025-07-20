#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QSharedPointer>
#include "datamodels.h"

namespace QtMyBatisORM {

class Session;

/**
 * Mapper注册表
 */
class MapperRegistry : public QObject
{
    Q_OBJECT
    
public:
    explicit MapperRegistry(QObject* parent = nullptr);
    
    void registerMapper(const QString& mapperName, const MapperConfig& config);
    void registerMappers(const QList<MapperConfig>& configs);
    
    // 从配置管理器加载Mapper
    bool loadMappersFromConfiguration();
    
    template<typename T>
    T* getMapper(QSharedPointer<Session> session);
    
    MapperConfig getMapperConfig(const QString& mapperName) const;
    bool hasMapper(const QString& mapperName) const;
    
    // 验证功能
    bool validateMapper(const QString& mapperName) const;
    bool validateAllMappers() const;
    
    // 查询功能
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
    // 从类型名获取Mapper名称
    QString typeName = T::staticMetaObject.className();
    // 移除命名空间前缀
    if (typeName.contains("::")) {
        typeName = typeName.split("::").last();
    }
    
    QString mapperName = getMapperNameFromType(typeName);
    
    // 创建或获取Mapper代理实例
    QString instanceKey = QString("%1_%2").arg(mapperName).arg(reinterpret_cast<quintptr>(session.data()));
    
    if (!m_mapperInstances.contains(instanceKey)) {
        QObject* proxy = createMapperProxy(mapperName, session);
        m_mapperInstances[instanceKey] = proxy;
    }
    
    return qobject_cast<T*>(m_mapperInstances[instanceKey]);
}

} // namespace QtMyBatisORM