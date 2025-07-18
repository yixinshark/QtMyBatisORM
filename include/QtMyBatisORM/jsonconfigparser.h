#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include "DataModels.h"
#include "export.h"

namespace QtMyBatisORM {

/**
 * JSON配置文件解析器
 */
class QTMYBATISORM_EXPORT JSONConfigParser : public QObject
{
    Q_OBJECT
    
public:
    explicit JSONConfigParser(QObject* parent = nullptr);
    
    DatabaseConfig parseConfiguration(const QString& resourcePath);
    
private:
    DatabaseConfig parseFromJsonObject(const QJsonObject& jsonObj);
    void validateConfiguration(const DatabaseConfig& config);
    QString readResourceFile(const QString& resourcePath);
};

} // namespace QtMyBatisORM