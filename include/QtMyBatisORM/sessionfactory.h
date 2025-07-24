#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QSet>
#include "datamodels.h"

namespace QtMyBatisORM {

class Session;
class ConnectionPool;
class MapperRegistry;
class CacheManager;

/**
 * Session factory
 */
class SessionFactory : public QObject
{
    Q_OBJECT
    
public:
    static QSharedPointer<SessionFactory> create(const DatabaseConfig& config);
    
    QSharedPointer<Session> openSession();
    void closeSession(QSharedPointer<Session> session);
    
    template<typename T>
    T* getMapper(QSharedPointer<Session> session);
    
    void close();
    bool isClosed() const;
    
    int getActiveSessionCount() const;
    
private:
    explicit SessionFactory(const DatabaseConfig& config, QObject* parent = nullptr);
    void initialize();
    
    DatabaseConfig m_config;
    QSharedPointer<ConnectionPool> m_connectionPool;
    QSharedPointer<MapperRegistry> m_mapperRegistry;
    QSharedPointer<CacheManager> m_cacheManager;
    
    QSet<QObject*> m_activeSessions;
    bool m_closed;
};

// Template method implementation will be provided in a separate header
// or through explicit instantiation

} // namespace QtMyBatisORM