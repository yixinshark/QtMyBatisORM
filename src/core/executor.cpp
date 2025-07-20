#include "QtMyBatisORM/executor.h"
#include "QtMyBatisORM/statementhandler.h"
#include "QtMyBatisORM/parameterhandler.h"
#include "QtMyBatisORM/resulthandler.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/qtmybatisexception.h"
#include "QtMyBatisORM/logger.h"
#include "QtMyBatisORM/objectpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QElapsedTimer>

namespace QtMyBatisORM {

// 参数处理器对象池
static ObjectPool<ParameterHandler> g_parameterHandlerPool(10, 20);

Executor::Executor(QSharedPointer<QSqlDatabase> connection, 
                  QSharedPointer<CacheManager> cacheManager,
                  QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_cacheManager(cacheManager)
    , m_debugMode(false)
{
    m_statementHandler = QSharedPointer<StatementHandler>::create();
    m_parameterHandler = QSharedPointer<ParameterHandler>::create();
    m_resultHandler = QSharedPointer<ResultHandler>::create();
    
    // 初始化SQL处理缓存
    m_processedSqlCache.reserve(200); // 预分配更多空间以提高性能
}

QVariant Executor::query(const QString& sql, const QVariantMap& parameters)
{
    return queryInternal(sql, parameters, QString(), false);
}

QVariantList Executor::queryList(const QString& sql, const QVariantMap& parameters)
{
    return queryListInternal(sql, parameters, QString(), false);
}

int Executor::update(const QString& sql, const QVariantMap& parameters)
{
    return updateInternal(sql, parameters, QString(), true);
}

QVariant Executor::queryWithCache(const QString& statementId, const QString& sql, 
                                 const QVariantMap& parameters)
{
    if (!m_cacheManager) {
        // 如果没有缓存管理器，直接执行查询
        return query(sql, parameters);
    }
    
    // 生成缓存键
    QString cacheKey = generateCacheKey(statementId, parameters);
    
    // 尝试从缓存获取结果
    QVariant cachedResult = m_cacheManager->get(cacheKey);
    if (!cachedResult.isNull()) {
        return cachedResult;
    }
    
    // 缓存中没有，执行查询
    QVariant result = query(sql, parameters);
    
    // 将结果存入缓存
    if (!result.isNull()) {
        m_cacheManager->put(cacheKey, result);
    }
    
    return result;
}

QVariantList Executor::queryListWithCache(const QString& statementId, const QString& sql, 
                                         const QVariantMap& parameters)
{
    if (!m_cacheManager) {
        // 如果没有缓存管理器，直接执行查询
        return queryList(sql, parameters);
    }
    
    // 生成缓存键
    QString cacheKey = generateCacheKey(statementId, parameters);
    
    // 尝试从缓存获取结果
    QVariant cachedResult = m_cacheManager->get(cacheKey);
    if (!cachedResult.isNull()) {
        return cachedResult.toList();
    }
    
    // 缓存中没有，执行查询
    QVariantList result = queryList(sql, parameters);
    
    // 将结果存入缓存
    if (!result.isEmpty()) {
        m_cacheManager->put(cacheKey, QVariant::fromValue(result));
    }
    
    return result;
}

int Executor::updateWithCacheInvalidation(const QString& statementId, const QString& sql, 
                                          const QVariantMap& parameters)
{
    return updateInternal(sql, parameters, statementId, true);
}

void Executor::clearCache(const QString& pattern)
{
    if (m_cacheManager) {
        if (pattern.isEmpty()) {
            m_cacheManager->clear();
        } else {
            m_cacheManager->invalidateByPattern(pattern);
        }
    }
}

QVariant Executor::queryInternal(const QString& sql, const QVariantMap& parameters, 
                                const QString& statementId, bool useCache)
{
    if (!m_connection || !m_connection->isOpen()) {
        throw ConnectionException(QStringLiteral("Database connection is not available"));
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // 如果启用缓存且有缓存管理器，尝试从缓存获取
    if (useCache && m_cacheManager && !statementId.isEmpty()) {
        QString cacheKey = generateCacheKey(statementId, parameters);
        QVariant cachedResult = m_cacheManager->get(cacheKey);
        if (!cachedResult.isNull()) {
            if (m_debugMode) {
                logDebugInfo("selectOne (缓存命中)", statementId.isEmpty() ? sql : statementId, 
                            parameters, timer.elapsed(), cachedResult);
            }
            return cachedResult;
        }
    }
    
    try {
        // 使用优化的方法获取处理后的SQL
        const QString processedSql = getProcessedSql(sql, parameters);
        
        // 准备查询
        QSqlQuery query = m_statementHandler->prepare(processedSql, *m_connection);
        
        // 使用对象池或回退策略绑定参数
        withParameterHandler(query, parameters);
        
        // 执行查询
        if (!query.exec()) {
            throw SqlExecutionException(
                QStringLiteral("Failed to execute query: %1. SQL: %2")
                .arg(query.lastError().text())
                .arg(processedSql)
            );
        }
        
        // 处理结果
        QVariant result = m_resultHandler->handleSingleResult(query);
        
        // 记录调试日志
        if (m_debugMode) {
            logDebugInfo("selectOne", statementId.isEmpty() ? processedSql : statementId, 
                        parameters, timer.elapsed(), result);
        }   
        
        // 如果启用缓存，将结果存入缓存
        if (useCache && m_cacheManager && !statementId.isEmpty() && !result.isNull()) {
            QString cacheKey = generateCacheKey(statementId, parameters);
            m_cacheManager->put(cacheKey, result);
        }
        
        return result;
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw SqlExecutionException(
            QStringLiteral("Unexpected error during query execution: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

QVariantList Executor::queryListInternal(const QString& sql, const QVariantMap& parameters, 
                                        const QString& statementId, bool useCache)
{
    if (!m_connection || !m_connection->isOpen()) {
        throw ConnectionException(QStringLiteral("Database connection is not available"));
    }
    
    // 如果启用缓存且有缓存管理器，尝试从缓存获取
    if (useCache && m_cacheManager && !statementId.isEmpty()) {
        QString cacheKey = generateCacheKey(statementId, parameters);
        QVariant cachedResult = m_cacheManager->get(cacheKey);
        if (!cachedResult.isNull()) {
            return cachedResult.toList();
        }
    }
    
    try {
        // 使用优化的方法获取处理后的SQL
        const QString processedSql = getProcessedSql(sql, parameters);
        
        // 准备查询
        QSqlQuery query = m_statementHandler->prepare(processedSql, *m_connection);
        
        // 使用对象池或回退策略绑定参数
        withParameterHandler(query, parameters);
        
        // 执行查询
        if (!query.exec()) {
            throw SqlExecutionException(
                QStringLiteral("Failed to execute query: %1. SQL: %2")
                .arg(query.lastError().text())
                .arg(processedSql)
            );
        }
        
        // 处理结果
        QVariantList result = m_resultHandler->handleListResult(query);
        
        // 如果启用缓存，将结果存入缓存
        if (useCache && m_cacheManager && !statementId.isEmpty() && !result.isEmpty()) {
            QString cacheKey = generateCacheKey(statementId, parameters);
            m_cacheManager->put(cacheKey, QVariant::fromValue(result));
        }
        
        return result;
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw SqlExecutionException(
            QStringLiteral("Unexpected error during query execution: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

int Executor::updateInternal(const QString& sql, const QVariantMap& parameters, 
                            const QString& statementId, bool invalidateCache)
{
    if (!m_connection || !m_connection->isOpen()) {
        throw ConnectionException(QStringLiteral("Database connection is not available"));
    }
    
    try {
        // 使用优化的方法获取处理后的SQL
        const QString processedSql = getProcessedSql(sql, parameters);
        
        // 准备查询
        QSqlQuery query = m_statementHandler->prepare(processedSql, *m_connection);
        
        // 使用对象池或回退策略绑定参数
        withParameterHandler(query, parameters);
        
        // 执行更新操作
        // Debug: Query execution
        // qDebug() << "[Executor] SQL:" << query.lastQuery();
        // qDebug() << "[Executor] Bound values:" << query.boundValues();
        
        if (!query.exec()) {
            // Debug: Query execution error
            // qDebug() << "[Executor] Error:" << query.lastError().text();
            
            throw SqlExecutionException(
                QStringLiteral("Failed to execute update: %1. SQL: %2")
                .arg(query.lastError().text())
                .arg(processedSql)
            );
        }
        
        int affectedRows = query.numRowsAffected();
        
        // 如果启用缓存失效且有受影响的行，清除相关缓存
        if (invalidateCache && m_cacheManager && affectedRows > 0) {
            invalidateCacheForStatement(statementId, sql);
        }
        
        return affectedRows;
        
    } catch (const QtMyBatisException&) {
        throw; // 重新抛出已知异常
    } catch (const std::exception& e) {
        throw SqlExecutionException(
            QStringLiteral("Unexpected error during update execution: %1").arg(QString::fromLatin1(e.what()))
        );
    }
}

void Executor::invalidateCacheForStatement(const QString& statementId, const QString& sql)
{
    if (!m_cacheManager) {
        return;
    }
    
    // 从SQL语句中提取表名，用于缓存失效
    QStringList tableNames = extractTableNamesFromSql(sql);
    
    // 为每个表名创建失效模式
    for (const QString& tableName : tableNames) {
        // 失效所有与该表相关的缓存条目
        QString pattern = QStringLiteral(".*%1.*").arg(tableName);
        m_cacheManager->invalidateByPattern(pattern);
    }
    
    // 如果有具体的语句ID，也失效相关的缓存
    if (!statementId.isEmpty()) {
        QString statementPattern = QStringLiteral("cache_%1_.*").arg(statementId.split(QLatin1Char('.')).first());
        m_cacheManager->invalidateByPattern(statementPattern);
    }
}

QStringList Executor::extractTableNamesFromSql(const QString& sql)
{
    // 使用静态缓存来存储常见SQL语句的表名
    static QMutex cacheMutex;
    static QHash<QString, QStringList> tableNameCache;
    
    // 首先检查缓存
    {
        QMutexLocker locker(&cacheMutex);
        if (tableNameCache.contains(sql)) {
            return tableNameCache[sql];
        }
        
        // 防止缓存无限增长
        if (tableNameCache.size() > 1000) {
            tableNameCache.clear();
        }
    }
    
    QStringList tableNames;
    QString upperSql = sql.toUpper();
    
    // 根据SQL类型进行更高效的解析
    if (upperSql.contains(QStringLiteral("SELECT"))) {
        // 解析SELECT语句
        int fromPos = upperSql.indexOf(QStringLiteral(" FROM "));
        if (fromPos > 0) {
            // 提取FROM后面直到下一个子句的文本
            int nextClausePos = upperSql.indexOf(QStringLiteral(" WHERE "), fromPos);
            if (nextClausePos < 0) nextClausePos = upperSql.indexOf(QStringLiteral(" GROUP "), fromPos);
            if (nextClausePos < 0) nextClausePos = upperSql.indexOf(QStringLiteral(" HAVING "), fromPos);
            if (nextClausePos < 0) nextClausePos = upperSql.indexOf(QStringLiteral(" ORDER "), fromPos);
            if (nextClausePos < 0) nextClausePos = upperSql.indexOf(QStringLiteral(" LIMIT "), fromPos);
            
            QString fromClause;
            if (nextClausePos > 0) {
                fromClause = upperSql.mid(fromPos + 6, nextClausePos - fromPos - 6).trimmed();
            } else {
                fromClause = upperSql.mid(fromPos + 6).trimmed();
            }
            
            // 处理简单表名和连接
            QStringList parts = fromClause.split(QLatin1Char(','));
            for (QString part : parts) {
                part = part.trimmed();
                // 处理表别名
                if (part.contains(QStringLiteral(" AS "))) {
                    part = part.left(part.indexOf(QStringLiteral(" AS "))).trimmed();
                } else if (part.contains(QLatin1Char(' '))) {
                    part = part.left(part.indexOf(QLatin1Char(' '))).trimmed();
                }
                tableNames.append(part.toLower());
            }
            
            // 处理JOIN子句
            QStringList joinKeywords = {QStringLiteral("JOIN"), QStringLiteral("INNER JOIN"), QStringLiteral("LEFT JOIN"), QStringLiteral("RIGHT JOIN"), QStringLiteral("FULL JOIN")};
            for (const QString& joinKeyword : joinKeywords) {
                int joinPos = 0;
                while ((joinPos = upperSql.indexOf(QStringLiteral(" ") + joinKeyword + QStringLiteral(" "), joinPos)) > 0) {
                    joinPos += joinKeyword.length() + 2; // 跳过关键字
                    int onPos = upperSql.indexOf(QStringLiteral(" ON "), joinPos);
                    if (onPos > 0) {
                        QString joinTable = upperSql.mid(joinPos, onPos - joinPos).trimmed();
                        if (joinTable.contains(QStringLiteral(" AS "))) {
                            joinTable = joinTable.left(joinTable.indexOf(QStringLiteral(" AS "))).trimmed();
                        } else if (joinTable.contains(QLatin1Char(' '))) {
                            joinTable = joinTable.left(joinTable.indexOf(QLatin1Char(' '))).trimmed();
                        }
                        tableNames.append(joinTable.toLower());
                    }
                }
            }
        }
    } else if (upperSql.contains(QStringLiteral("INSERT"))) {
        // 处理INSERT语句
        int intoPos = upperSql.indexOf(QStringLiteral(" INTO "));
        if (intoPos > 0) {
            int valuesPos = upperSql.indexOf(QStringLiteral(" VALUES "), intoPos);
            if (valuesPos < 0) valuesPos = upperSql.indexOf(QStringLiteral(" SELECT "), intoPos);
            if (valuesPos < 0) valuesPos = upperSql.length();
            
            QString tablePart = upperSql.mid(intoPos + 6, valuesPos - intoPos - 6).trimmed();
            if (tablePart.contains(QLatin1Char('('))) {
                tablePart = tablePart.left(tablePart.indexOf(QLatin1Char('('))).trimmed();
            }
            tableNames.append(tablePart.toLower());
        }
    } else if (upperSql.contains(QStringLiteral("UPDATE"))) {
        // 处理UPDATE语句
        int updatePos = upperSql.indexOf(QStringLiteral("UPDATE "));
        if (updatePos >= 0) {
            int setPos = upperSql.indexOf(QStringLiteral(" SET "), updatePos);
            if (setPos > 0) {
                QString tablePart = upperSql.mid(updatePos + 7, setPos - updatePos - 7).trimmed();
                tableNames.append(tablePart.toLower());
            }
        }
    } else if (upperSql.contains(QStringLiteral("DELETE"))) {
        // 处理DELETE语句
        int fromPos = upperSql.indexOf(QStringLiteral(" FROM "));
        if (fromPos > 0) {
            int wherePos = upperSql.indexOf(QStringLiteral(" WHERE "), fromPos);
            if (wherePos < 0) wherePos = upperSql.length();
            
            QString tablePart = upperSql.mid(fromPos + 6, wherePos - fromPos - 6).trimmed();
            tableNames.append(tablePart.toLower());
        }
    } else {
        // 回退到正则表达式方法处理其他类型的SQL
        QRegularExpression fromRegex(QStringLiteral(R"(\bFROM\s+(\w+))"));
        QRegularExpression insertRegex(QStringLiteral(R"(\bINSERT\s+INTO\s+(\w+))"));
        QRegularExpression updateRegex(QStringLiteral(R"(\bUPDATE\s+(\w+))"));
        QRegularExpression deleteRegex(QStringLiteral(R"(\bDELETE\s+FROM\s+(\w+))"));
        
        // 提取表名
        auto addMatches = [&tableNames](QRegularExpressionMatchIterator matches) {
            while (matches.hasNext()) {
                QRegularExpressionMatch match = matches.next();
                tableNames.append(match.captured(1).toLower());
            }
        };
        
        addMatches(fromRegex.globalMatch(upperSql));
        addMatches(insertRegex.globalMatch(upperSql));
        addMatches(updateRegex.globalMatch(upperSql));
        addMatches(deleteRegex.globalMatch(upperSql));
    }
    
    // 去重
    tableNames.removeDuplicates();
    
    // 缓存结果
    {
        QMutexLocker locker(&cacheMutex);
        tableNameCache[sql] = tableNames;
    }
    
    return tableNames;
}

QString Executor::generateCacheKey(const QString& statementId, const QVariantMap& parameters)
{
    // 使用更高效的字符串拼接方法
    QString keyBase = statementId;
    
    // 对参数键进行排序以确保一致的顺序
    QStringList keys = parameters.keys();
    std::sort(keys.begin(), keys.end());
    
    // 将参数附加到键
    for (const QString& key : keys) {
        keyBase.append(QLatin1Char('|')).append(key).append(QLatin1Char('='));
        const QVariant& value = parameters[key];
        
        // 直接处理常见类型，避免转换为字符串
        switch (value.typeId()) {
            case QMetaType::Int:
                keyBase.append(QString::number(value.toInt()));
                break;
            case QMetaType::LongLong:
                keyBase.append(QString::number(value.toLongLong()));
                break;
            case QMetaType::Double:
                keyBase.append(QString::number(value.toDouble(), 'f', 6));
                break;
            case QMetaType::Bool:
                keyBase.append(value.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                break;
            case QMetaType::QDateTime:
                keyBase.append(value.toDateTime().toString(Qt::ISODate));
                break;
            case QMetaType::QDate:
                keyBase.append(value.toDate().toString(Qt::ISODate));
                break;
            case QMetaType::QTime:
                keyBase.append(value.toTime().toString(Qt::ISODate));
                break;
            default:
                keyBase.append(value.toString());
                break;
        }
    }
    
    // 使用更快的FNV-1a哈希算法代替MD5
    uint32_t hash = 2166136261; // FNV偏移基数
    QByteArray data = keyBase.toUtf8();
    for (char c : data) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619; // FNV素数
    }
    
    return QStringLiteral("cache_%1_%2").arg(statementId).arg(hash, 8, 16, QLatin1Char('0'));
}

QString Executor::getProcessedSql(const QString& sql, const QVariantMap& parameters)
{
    // 对于无参数查询或简单查询，使用缓存
    if (parameters.isEmpty() || (!sql.contains(QStringLiteral("${")) && !sql.contains(QStringLiteral("#}")))) {
        QMutexLocker locker(&m_sqlCacheMutex);
        if (m_processedSqlCache.contains(sql)) {
            return m_processedSqlCache[sql];
        }
        
        // 处理SQL并缓存
        QString processedSql = m_statementHandler->processSql(sql, parameters);
        
        // 限制缓存大小，防止无限增长
        if (m_processedSqlCache.size() >= 1000) {
            // 如果缓存太大，清除一半
            QList<QString> keys = m_processedSqlCache.keys();
            for (int i = 0; i < keys.size() / 2; ++i) {
                m_processedSqlCache.remove(keys[i]);
            }
        }
        
        m_processedSqlCache[sql] = processedSql;
        return processedSql;
    }
    
    // 对于带参数的动态SQL，不缓存直接处理
    return m_statementHandler->processSql(sql, parameters);
}

void Executor::withParameterHandler(QSqlQuery &query, const QVariantMap &parameters)
 {
    // 从对象池获取参数处理器并绑定参数
    ParameterHandler* paramHandler = g_parameterHandlerPool.acquire();
    if (paramHandler) {
        // 使用对象池中的处理器
        paramHandler->setParameters(query, parameters);
        g_parameterHandlerPool.release(paramHandler);
    } else {
        // 如果对象池已满，回退到成员变量
        m_parameterHandler->setParameters(query, parameters);
    }
}

void Executor::setDebugMode(bool enabled)
{
    m_debugMode = enabled;
}

bool Executor::isDebugMode() const
{
    return m_debugMode;
}

void Executor::logDebugInfo(const QString& operation, const QString& sql, 
                           const QVariantMap& parameters, qint64 elapsedMs, 
                           const QVariant& result) const
{
    if (!m_debugMode) {
        return;
    }
    
    QString paramStr;
    if (!parameters.isEmpty()) {
        QStringList paramList;
        for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
            paramList << QString("%1=%2").arg(it.key()).arg(it.value().toString());
        }
        paramStr = QString(" 参数:[%1]").arg(paramList.join(", "));
    }
    
    QString resultStr;
    if (result.isValid()) {
        if (result.canConvert<QVariantList>()) {
            resultStr = QString(" 结果:[返回%1条记录]").arg(result.toList().size());
        } else if (result.canConvert<QVariantMap>()) {
            QVariantMap map = result.toMap();
            resultStr = QString(" 结果:[对象包含%1个字段]").arg(map.size());
        } else {
            resultStr = QString(" 结果:[%1]").arg(result.toString());
        }
    }
    
    qDebug() << QString("[QtMyBatisORM DEBUG] %1: %2%3%4 耗时:%5ms")
                .arg(operation)
                .arg(sql)
                .arg(paramStr)
                .arg(resultStr)
                .arg(elapsedMs);
}

} // namespace QtMyBatisORM