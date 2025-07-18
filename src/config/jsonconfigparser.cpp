#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QResource>
#include <QDebug>

namespace QtMyBatisORM {

JSONConfigParser::JSONConfigParser(QObject* parent)
    : QObject(parent)
{
}

DatabaseConfig JSONConfigParser::parseConfiguration(const QString& configPath)
{
    try {
        QString configContent = readResourceFile(configPath);
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(configContent.toUtf8(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            throw ConfigurationException(
                QStringLiteral("JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString())
            );
        }
        
        if (!doc.isObject()) {
            throw ConfigurationException(QStringLiteral("JSON configuration must be an object"));
        }
        
        DatabaseConfig config = parseFromJsonObject(doc.object());
        validateConfiguration(config);
        
        return config;
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    }
}

DatabaseConfig JSONConfigParser::parseFromJsonObject(const QJsonObject& jsonObj)
{
    DatabaseConfig config;
    
    // 解析数据库驱动配置
    config.driverName = jsonObj.value(QStringLiteral("driverName")).toString(
        jsonObj.value(QStringLiteral("driver")).toString(QStringLiteral("QSQLITE"))
    );
    config.hostName = jsonObj.value(QStringLiteral("hostName")).toString(
        jsonObj.value(QStringLiteral("host")).toString(QStringLiteral("localhost"))
    );
    config.port = jsonObj.value(QStringLiteral("port")).toInt(3306);
    config.databaseName = jsonObj.value(QStringLiteral("databaseName")).toString(
        jsonObj.value(QStringLiteral("database")).toString()
    );
    config.userName = jsonObj.value(QStringLiteral("userName")).toString(
        jsonObj.value(QStringLiteral("username")).toString()
    );
    config.password = jsonObj.value(QStringLiteral("password")).toString();
    
    // 解析连接池配置
    QJsonObject poolConfig = jsonObj.value(QStringLiteral("connectionPool")).toObject();
    if (poolConfig.isEmpty()) {
        // 使用顶层配置作为回退
        config.maxConnections = jsonObj.value(QStringLiteral("maxConnections")).toInt(10);
        config.minConnections = jsonObj.value(QStringLiteral("minConnections")).toInt(2);
        config.maxIdleTime = jsonObj.value(QStringLiteral("maxIdleTime")).toInt(300);
    } else {
        // 使用连接池对象中的配置
        config.maxConnections = poolConfig.value(QStringLiteral("maxConnections")).toInt(10);
        config.minConnections = poolConfig.value(QStringLiteral("minConnections")).toInt(2);
        config.maxIdleTime = poolConfig.value(QStringLiteral("maxIdleTime")).toInt(300);
    }
    
    // 解析缓存配置
    QJsonObject cacheConfig = jsonObj.value(QStringLiteral("cache")).toObject();
    if (cacheConfig.isEmpty()) {
        // 使用顶层配置作为回退
        config.cacheEnabled = jsonObj.value(QStringLiteral("cacheEnabled")).toBool(true);
        config.maxCacheSize = jsonObj.value(QStringLiteral("maxCacheSize")).toInt(1000);
        config.cacheExpireTime = jsonObj.value(QStringLiteral("cacheExpireTime")).toInt(600);
    } else {
        // 使用缓存对象中的配置
        config.cacheEnabled = cacheConfig.value(QStringLiteral("enabled")).toBool(true);
        config.maxCacheSize = cacheConfig.value(QStringLiteral("maxSize")).toInt(1000);
        config.cacheExpireTime = cacheConfig.value(QStringLiteral("expireTime")).toInt(600);
    }
    
    return config;
}

void JSONConfigParser::validateConfiguration(const DatabaseConfig& config)
{
    // 验证必需字段
    if (config.driverName.isEmpty()) {
        throw ConfigurationException(QStringLiteral("Database driver name cannot be empty"));
    }
    
    if (config.driverName != QStringLiteral("QMYSQL") && config.driverName != QStringLiteral("QSQLITE")) {
        throw ConfigurationException(
            QStringLiteral("Unsupported database driver: %1. Only QMYSQL and QSQLITE are supported")
            .arg(config.driverName)
        );
    }
    
    // 对于SQLite以外的数据库，数据库名是必需的
    if (config.driverName != QStringLiteral("QSQLITE") && config.databaseName.isEmpty()) {
        throw ConfigurationException(QStringLiteral("Database name cannot be empty"));
    }
    
    // 验证连接池配置
    if (config.maxConnections <= 0) {
        throw ConfigurationException(QStringLiteral("Max connections must be greater than 0"));
    }
    
    if (config.minConnections < 0) {
        throw ConfigurationException(QStringLiteral("Min connections cannot be negative"));
    }
    
    if (config.minConnections > config.maxConnections) {
        throw ConfigurationException(QStringLiteral("Min connections cannot be greater than max connections"));
    }
}

QString JSONConfigParser::readResourceFile(const QString& resourcePath)
{
    // 首先尝试作为常规文件读取
    QFile file(resourcePath);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }
    
    // 然后尝试作为Qt资源文件读取
    QResource resource(resourcePath);
    if (resource.isValid()) {
        QByteArray data(reinterpret_cast<const char*>(resource.data()), resource.size());
        return QString::fromUtf8(data);
    }
    
    // 如果都失败了，抛出异常
    throw ConfigurationException(
        QStringLiteral("Configuration file not found: %1. Tried both regular file and resource file")
        .arg(resourcePath)
    );
}

} // namespace QtMyBatisORM