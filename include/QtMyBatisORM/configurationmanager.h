#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QMutex>
#include "datamodels.h"
#include "export.h"

namespace QtMyBatisORM {

class JSONConfigParser;
class XMLMapperParser;

/**
 * 配置管理器 - 单例模式
 */
class QTMYBATISORM_EXPORT ConfigurationManager : public QObject
{
    Q_OBJECT
    
public:
    static ConfigurationManager* instance();
    
    bool loadConfiguration(const QString& configPath);
    bool loadMappers(const QStringList& mapperPaths);
    
    DatabaseConfig getDatabaseConfig() const;
    QList<MapperConfig> getMapperConfigs() const;
    MapperConfig getMapperConfig(const QString& namespace_) const;
    
    bool hasMapper(const QString& namespace_) const;
    void reset();
    
    // 便利方法
    bool isConfigured() const;
    int getMapperCount() const;
    QStringList getMapperNamespaces() const;
    
private:
    explicit ConfigurationManager(QObject* parent = nullptr);
    ~ConfigurationManager();
    
    static ConfigurationManager* s_instance;
    static QMutex s_mutex;
    
    DatabaseConfig m_dbConfig;
    QHash<QString, MapperConfig> m_mappers;
    
    JSONConfigParser* m_jsonParser;
    XMLMapperParser* m_xmlParser;
    
    mutable QMutex m_configMutex;
};

} // namespace QtMyBatisORM