#include "QtMyBatisORM/resulthandler.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>

namespace QtMyBatisORM {

ResultHandler::ResultHandler(QObject* parent)
    : QObject(parent)
{
}

QVariant ResultHandler::handleSingleResult(QSqlQuery& query)
{
    try {
        if (!query.isActive()) {
            if (query.lastError().isValid()) {
                throw SqlExecutionException(
                    QStringLiteral("Query error before processing single result: %1")
                    .arg(query.lastError().text())
                );
            }
            // 查询未激活但没有错误，返回空结果
            return QVariant();
        }
        
        if (!query.next()) {
            // 没有结果
            return QVariant();
        }
        
        QSqlRecord record = query.record();
        
        // 对于单列查询（通常是标量值，如COUNT、MAX、MIN等），直接返回值
        if (record.count() == 1) {
            QVariant fieldValue = normalizeValue(query.value(0));
            return fieldValue;
        }
        
        // 对于多列查询，返回QVariantMap
        QVariantMap resultMap;
        for (int i = 0; i < record.count(); ++i) {
            QString fieldName = record.fieldName(i);
            QVariant fieldValue = normalizeValue(query.value(i));
            resultMap[fieldName] = fieldValue;
        }
        
        return resultMap;
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw SqlExecutionException(
            QStringLiteral("Unexpected error during single result processing: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

QVariantList ResultHandler::handleListResult(QSqlQuery& query)
{
    try {
        QVariantList results;
        
        if (!query.isActive()) {
            if (query.lastError().isValid()) {
                throw SqlExecutionException(
                    QStringLiteral("Query error before processing results: %1")
                    .arg(query.lastError().text())
                );
            }
            // 查询未激活但没有错误，返回空列表
            return results;
        }
        
        // 性能优化：预分配空间和结果集大小检查
        const int maxRows = 10000; // 防止过大的结果集
        int rowCount = 0;
        
        while (query.next() && rowCount < maxRows) {
            QSqlRecord record = query.record();
            QVariantMap resultMap;
            
            for (int i = 0; i < record.count(); ++i) {
                QString fieldName = record.fieldName(i);
                QVariant fieldValue = normalizeValue(query.value(i));
                resultMap[fieldName] = fieldValue;
            }
            
            results.append(resultMap);
            rowCount++;
            
            if (rowCount >= maxRows) {
                qWarning() << QStringLiteral("Result set too large, limiting to %1 rows").arg(maxRows);
                break;
            }
        }
        
        // 检查是否有查询错误
        if (query.lastError().isValid()) {
            throw SqlExecutionException(
                QStringLiteral("Query error during result processing: %1")
                .arg(query.lastError().text())
            );
        }
        
        return results;
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw SqlExecutionException(
            QStringLiteral("Unexpected error during list result processing: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

QVariant ResultHandler::convertFromSqlType(const QVariant& value, const QString& targetType)
{
    if (value.isNull()) {
        return QVariant();
    }
    
    try {
        if (targetType.isEmpty()) {
            return normalizeValue(value);
        }
        
        // 根据目标类型进行特定转换
        if (targetType == QStringLiteral("int") || targetType == QStringLiteral("integer")) {
            return value.toInt();
        } else if (targetType == QStringLiteral("long") || targetType == QStringLiteral("bigint")) {
            return value.toLongLong();
        } else if (targetType == QStringLiteral("string") || targetType == QStringLiteral("varchar") || targetType == QStringLiteral("text")) {
            return value.toString();
        } else if (targetType == QStringLiteral("double") || targetType == QStringLiteral("float") || targetType == QStringLiteral("decimal")) {
            return value.toDouble();
        } else if (targetType == QStringLiteral("bool") || targetType == QStringLiteral("boolean")) {
            return value.toBool();
        } else if (targetType == QStringLiteral("date")) {
            return value.toDate();
        } else if (targetType == QStringLiteral("datetime") || targetType == QStringLiteral("timestamp")) {
            return value.toDateTime();
        } else if (targetType == QStringLiteral("time")) {
            return value.toTime();
        } else if (targetType == QStringLiteral("uuid")) {
            return value.toUuid();
        } else if (targetType == QStringLiteral("json") || targetType == QStringLiteral("jsonb")) {
            // JSON类型转换
            return QJsonDocument::fromJson(value.toByteArray()).toVariant();
        } else if (targetType == QStringLiteral("array")) {
            // 数组类型转换
            return value.toList();
        } else if (targetType == QStringLiteral("binary") || targetType == QStringLiteral("blob")) {
            return value.toByteArray();
        }
        
        return normalizeValue(value);
        
    } catch (const std::exception& e) {
        qWarning() << QStringLiteral("Error converting value to type %1: %2").arg(targetType, QString::fromLatin1(e.what()));
        return value;
    }
}

QVariant ResultHandler::normalizeValue(const QVariant& value)
{
    // 标准化数据库值，确保类型一致性
    if (value.isNull()) {
        return QVariant();
    }
    
    switch (value.typeId()) {
        case QMetaType::QString: {
            QString str = value.toString();
            // 处理空字符串
            if (str.isEmpty()) {
                return QVariant();
            }
            return str;
        }
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
            return value.toUuid();
        default:
            return value;
    }
}

QVariantMap ResultHandler::recordToMap(const QSqlQuery& query)
{
    QVariantMap record;
    QSqlRecord sqlRecord = query.record();
    
    for (int i = 0; i < sqlRecord.count(); ++i) {
        QString fieldName = sqlRecord.fieldName(i);
        QVariant fieldValue = query.value(i);
        record[fieldName] = normalizeValue(fieldValue);
    }
    
    return record;
}

QVariantMap ResultHandler::extractRecord(const QSqlQuery& query)
{
    return recordToMap(query);
}

QStringList ResultHandler::getColumnNames(const QSqlQuery& query)
{
    QStringList columnNames;
    QSqlRecord record = query.record();
    
    for (int i = 0; i < record.count(); ++i) {
        columnNames.append(record.fieldName(i));
    }
    
    return columnNames;
}

QVariant ResultHandler::parseJsonValue(const QString& jsonString)
{
    if (jsonString.isEmpty()) {
        return QVariant();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON:" << error.errorString();
        return jsonString; // 返回原始字符串
    }
    
    if (doc.isObject()) {
        return doc.object().toVariantMap();
    } else if (doc.isArray()) {
        return doc.array().toVariantList();
    } else {
        return jsonString; // 返回原始字符串
    }
}

QVariantList ResultHandler::parseJsonArray(const QString& jsonString)
{
    if (jsonString.isEmpty()) {
        return QVariantList();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON array:" << error.errorString();
        return QVariantList();
    }
    
    if (doc.isArray()) {
        return doc.array().toVariantList();
    } else {
        qWarning() << "JSON string is not an array";
        return QVariantList();
    }
}

} // namespace QtMyBatisORM