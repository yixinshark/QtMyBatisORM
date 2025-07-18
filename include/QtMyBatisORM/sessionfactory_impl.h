#pragma once

#include "sessionfactory.h"
#include "session.h"
#include "mapperregistry.h"

namespace QtMyBatisORM {

// Template method implementation
template<typename T>
T* SessionFactory::getMapper(QSharedPointer<Session> session)
{
    if (m_closed) {
        return nullptr;
    }
    
    if (!session || session->isClosed()) {
        return nullptr;
    }
    
    if (!m_mapperRegistry) {
        return nullptr;
    }
    
    return m_mapperRegistry->template getMapper<T>(session);
}

} // namespace QtMyBatisORM