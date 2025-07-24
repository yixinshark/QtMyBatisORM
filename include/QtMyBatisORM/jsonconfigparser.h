#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include "datamodels.h"
#include "export.h"

namespace QtMyBatisORM {

/**
 * JSON configuration file parser
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