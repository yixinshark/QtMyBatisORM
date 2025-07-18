#pragma once

#include "session.h"
#include "mapperregistry.h"
#include "mapperproxy.h"
#include <QHash>

namespace QtMyBatisORM {

// Template method implementation for Session::getMapper
template<typename T>
T* Session::getMapper()
{
    checkClosed();
    
    if (!m_mapperRegistry) {
        throw MappingException("MapperRegistry is not available");
    }
    
    // Create a shared pointer to this session for the mapper
    // Use a custom deleter that does nothing since the session is managed elsewhere
    QSharedPointer<Session> sessionPtr(this, [](Session*){});
    
    // Get the mapper name from the template type
    QString typeName = T::staticMetaObject.className();
    if (typeName.contains("::")) {
        typeName = typeName.split("::").last();
    }
    
    QString mapperName = typeName.endsWith("Mapper") ? typeName : typeName + "Mapper";
    
    // Create instance key for caching
    QString instanceKey = QString("%1_%2").arg(mapperName).arg(reinterpret_cast<quintptr>(this));
    
    // Check if we already have this mapper instance
    static QHash<QString, QObject*> mapperInstances;
    if (mapperInstances.contains(instanceKey)) {
        return qobject_cast<T*>(mapperInstances[instanceKey]);
    }
    
    // Create new mapper proxy
    if (!m_mapperRegistry->hasMapper(mapperName)) {
        throw MappingException(QString("Mapper not registered: %1").arg(mapperName));
    }
    
    MapperConfig config = m_mapperRegistry->getMapperConfig(mapperName);
    QObject* proxy = new MapperProxy(mapperName, sessionPtr, config);
    
    // Cache the instance
    mapperInstances[instanceKey] = proxy;
    
    return qobject_cast<T*>(proxy);
}

} // namespace QtMyBatisORM