#include "QtMyBatisORM/parameterhandler.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QRegularExpression>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace QtMyBatisORM {

ParameterHandler::ParameterHandler(QObject* parent)
    : QObject(parent)
{
}

void ParameterHandler::setParameters(QSqlQuery& query, const QVariantMap& parameters)
{
    try {
        // 检查查询是否使用命名参数或位置参数
        QString sql = query.lastQuery();
        if (sql.isEmpty()) {
            throw MappingException(QStringLiteral("Query SQL is empty, cannot bind parameters"));
        }
        
        QRegularExpression namedParamRegex(QStringLiteral(R"(:\w+)"));
        bool hasNamedParams = sql.contains(namedParamRegex);
        bool hasPositionalParams = sql.contains(QStringLiteral("?"));
        
        if (hasNamedParams) {
            bindByName(query, parameters);
        } else if (hasPositionalParams) {
            bindByIndex(query, parameters);
        } else if (!parameters.isEmpty()) {
            // SQL中没有参数占位符但提供了参数，记录警告
            qWarning() << "Parameters provided but no placeholders found in SQL:" << sql;
        }
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw MappingException(
            QStringLiteral("Unexpected error during parameter binding: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

QVariant ParameterHandler::convertParameter(const QVariant& value, const QString& targetType)
{
    if (targetType.isEmpty()) {
        return convertToSqlType(value);
    }
    
    // 根据目标类型转换
    if (targetType == QStringLiteral("int") || targetType == QStringLiteral("integer")) {
        return value.toInt();
    } else if (targetType == QStringLiteral("string") || targetType == QStringLiteral("varchar")) {
        return value.toString();
    } else if (targetType == QStringLiteral("double") || targetType == QStringLiteral("float")) {
        return value.toDouble();
    } else if (targetType == QStringLiteral("bool") || targetType == QStringLiteral("boolean")) {
        return value.toBool();
    } else if (targetType == QStringLiteral("date") || targetType == QStringLiteral("datetime")) {
        return value.toDateTime();
    }
    
    return convertToSqlType(value);
}

void ParameterHandler::bindByIndex(QSqlQuery& query, const QVariantMap& parameters)
{
    try {
        // 计算SQL中的占位符数量
        QString sql = query.lastQuery();
        int placeholderCount = sql.count(QLatin1Char('?'));
        
        if (placeholderCount == 0 && !parameters.isEmpty()) {
            throw MappingException(QStringLiteral("No positional placeholders found in SQL but parameters provided"));
        }
        
        if (placeholderCount != parameters.size()) {
            throw MappingException(
                QStringLiteral("Parameter count mismatch: SQL has %1 placeholders but %2 parameters provided")
                .arg(placeholderCount)
                .arg(parameters.size())
            );
        }
        
        // 按索引绑定参数（使用数字键或按字母顺序）
        QStringList keys = parameters.keys();
        
        // 尝试按数字键排序，如果失败则按字母顺序
        bool hasNumericKeys = true;
        for (const QString& key : keys) {
            bool ok;
            key.toInt(&ok);
            if (!ok) {
                hasNumericKeys = false;
                break;
            }
        }
        
        if (hasNumericKeys) {
            // 按数字键排序
            std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
                return a.toInt() < b.toInt();
            });
        } else {
            // 按字母顺序排序
            keys.sort();
        }
        
        for (int i = 0; i < keys.size(); ++i) {
            QVariant value = convertToSqlType(parameters[keys[i]]);
            query.bindValue(i, value);
        }
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw MappingException(
            QStringLiteral("Error binding parameters by index: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

void ParameterHandler::bindByName(QSqlQuery& query, const QVariantMap& parameters)
{
    try {
        QString sql = query.lastQuery();
        
        // 提取SQL中的所有命名参数
        QRegularExpression re(QStringLiteral(R"(:(\w+))"));
        QRegularExpressionMatchIterator it = re.globalMatch(sql);
        QStringList sqlParameters;
        
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString paramName = match.captured(1);
            if (!sqlParameters.contains(paramName)) {
                sqlParameters.append(paramName);
            }
        }
        
        // 检查是否有未提供的必需参数
        QStringList missingParams;
        for (const QString& sqlParam : sqlParameters) {
            if (!parameters.contains(sqlParam)) {
                missingParams.append(sqlParam);
            }
        }
        
        if (!missingParams.isEmpty()) {
            throw MappingException(
                QStringLiteral("Missing required parameters: %1").arg(missingParams.join(QStringLiteral(", ")))
            );
        }
        
        // 检查是否有多余的参数
        QStringList extraParams;
        for (auto paramIt = parameters.begin(); paramIt != parameters.end(); ++paramIt) {
            if (!sqlParameters.contains(paramIt.key())) {
                extraParams.append(paramIt.key());
            }
        }
        
        if (!extraParams.isEmpty()) {
            qWarning() << "Extra parameters provided (will be ignored):" << extraParams.join(QStringLiteral(", "));
        }
        
        // 按名称绑定参数
        for (auto paramIt = parameters.begin(); paramIt != parameters.end(); ++paramIt) {
            QString paramName = paramIt.key();
            QString fullParamName = paramName.startsWith(QLatin1Char(':')) ? paramName : QStringLiteral(":") + paramName;
            
            if (isValidParameterName(fullParamName) && sqlParameters.contains(paramName)) {
                QVariant value = convertToSqlType(paramIt.value());
                query.bindValue(fullParamName, value);
            }
        }
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw MappingException(
            QStringLiteral("Error binding parameters by name: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

QVariant ParameterHandler::convertToSqlType(const QVariant& value)
{
    // 确保值适合SQL数据库
    switch (value.typeId()) {
        case QMetaType::UnknownType:
            return QVariant();
        case QMetaType::QString:
            return value.toString();
        case QMetaType::Int:
        case QMetaType::UInt:
            return value.toInt();
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            return value.toLongLong();
        case QMetaType::Double:
            return value.toDouble();
        case QMetaType::Bool:
            return value.toBool();
        case QMetaType::QDate:
            return value.toDate();
        case QMetaType::QDateTime:
            return value.toDateTime();
        case QMetaType::QTime:
            return value.toTime();
        case QMetaType::QByteArray:
            return value.toByteArray();
        case QMetaType::QUuid:
            return value.toUuid().toString();
        case QMetaType::QVariantList:
        case QMetaType::QStringList: {
            // 将列表转换为JSON字符串
            QJsonArray jsonArray;
            QVariantList list = value.toList();
            for (const QVariant& item : list) {
                if (item.canConvert<QString>()) {
                    jsonArray.append(item.toString());
                } else if (item.canConvert<int>()) {
                    jsonArray.append(item.toInt());
                } else if (item.canConvert<double>()) {
                    jsonArray.append(item.toDouble());
                } else if (item.canConvert<bool>()) {
                    jsonArray.append(item.toBool());
                } else {
                    jsonArray.append(item.toString());
                }
            }
            QJsonDocument doc(jsonArray);
            return doc.toJson(QJsonDocument::Compact);
        }
        case QMetaType::QVariantMap: {
            // 将Map转换为JSON字符串
            QVariantMap map = value.toMap();
            QJsonObject jsonObject;
            for (auto it = map.begin(); it != map.end(); ++it) {
                QVariant mapValue = it.value();
                if (mapValue.typeId() == QMetaType::Bool) {
                    jsonObject[it.key()] = mapValue.toBool();
                } else if (mapValue.typeId() == QMetaType::Int || mapValue.typeId() == QMetaType::LongLong) {
                    jsonObject[it.key()] = mapValue.toLongLong();
                } else if (mapValue.typeId() == QMetaType::Double) {
                    jsonObject[it.key()] = mapValue.toDouble();
                } else if (mapValue.canConvert<QString>()) {
                    jsonObject[it.key()] = mapValue.toString();
                } else {
                    jsonObject[it.key()] = mapValue.toString();
                }
            }
            QJsonDocument doc(jsonObject);
            return doc.toJson(QJsonDocument::Compact);
        }
        default:
            // 对于未知类型，尝试转换为字符串
            if (value.canConvert<QString>()) {
                return value.toString();
            }
            return QVariant();
    }
}

bool ParameterHandler::isValidParameterName(const QString& name)
{
    // 检查参数名是否有效：必须以冒号开头，后跟字母或下划线，然后可以跟字母、数字或下划线
    QRegularExpression re(QStringLiteral(R"(^:[a-zA-Z_][a-zA-Z0-9_]*$)"));
    return re.match(name).hasMatch();
}

} // namespace QtMyBatisORM