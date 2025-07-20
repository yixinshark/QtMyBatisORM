#include "QtMyBatisORM/jsonconfigparser.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
    
    // 新格式要求包含database对象
    if (!jsonObj.contains(QStringLiteral("database"))) {
        throw ConfigurationException(QStringLiteral("JSON configuration must contain 'database' object"));
    }
    
    QJsonObject dbConfig = jsonObj.value(QStringLiteral("database")).toObject();
    if (dbConfig.isEmpty()) {
        throw ConfigurationException(QStringLiteral("Database configuration object cannot be empty"));
    }
    
    // 解析数据库驱动配置
    config.driverName = dbConfig.value(QStringLiteral("type")).toString(QStringLiteral("QSQLITE"));
    config.hostName = dbConfig.value(QStringLiteral("host")).toString(QStringLiteral("localhost"));
    config.port = dbConfig.value(QStringLiteral("port")).toInt(3306);
    config.databaseName = dbConfig.value(QStringLiteral("database_name")).toString();
    config.userName = dbConfig.value(QStringLiteral("username")).toString();
    config.password = dbConfig.value(QStringLiteral("password")).toString();
    
    // 解析调试配置
    config.debug = dbConfig.value(QStringLiteral("debug")).toBool(false);
    
    // 解析连接池配置
    config.maxConnections = dbConfig.value(QStringLiteral("max_connection_count")).toInt(10);
    config.minConnections = 2; // 固定值，简化配置
    config.maxWaitTime = dbConfig.value(QStringLiteral("max_wait_time")).toInt(5000);
    config.maxIdleTime = 300; // 固定值，简化配置
    
    // 解析SQL文件列表
    QJsonArray sqlFilesArray = dbConfig.value(QStringLiteral("sql_files")).toArray();
    for (const QJsonValue& value : sqlFilesArray) {
        QString sqlFile = value.toString();
        if (!sqlFile.isEmpty()) {
            config.sqlFiles.append(sqlFile);
        }
    }
    
    // 缓存配置使用默认值，简化配置
    config.cacheEnabled = true;
    config.maxCacheSize = 1000;
    config.cacheExpireTime = 600;
    
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