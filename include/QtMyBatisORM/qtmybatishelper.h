#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QSharedPointer>
#include <functional>
#include "export.h"

namespace QtMyBatisORM {

class QtMyBatisORM;
class Session;

/**
 * Static database operation utility class
 * Fully encapsulates ORM and Session management, providing a clean static interface
 * 静态数据库操作工具类
 * 完全封装ORM和Session管理，提供简洁的静态接口
 */
class QTMYBATISORM_EXPORT QtMyBatisHelper
{
public:
    // Initialization methods
    static bool initialize(const QString& configResourcePath);
    static void shutdown();
    static bool isInitialized();
    
    // Basic CRUD operations - automatically manages Session lifecycle internally
    static QVariant selectOne(const QString& statementId, const QVariantMap& parameters = {});
    static QVariantList selectList(const QString& statementId, const QVariantMap& parameters = {});
    static int insert(const QString& statementId, const QVariantMap& parameters = {});
    static int update(const QString& statementId, const QVariantMap& parameters = {});
    static int remove(const QString& statementId, const QVariantMap& parameters = {});
    static int execute(const QString& sql, const QVariantMap& parameters = {});
    
    // Batch operations
    static int batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList);
    static int batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList);
    static int batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList);
    
    // Transaction operations - ensures Session is properly closed
    static bool executeInTransaction(std::function<bool()> operation);
    static bool executeInTransaction(std::function<bool(QSharedPointer<Session>)> operation);
    
    // Debug and monitoring
    static void setDebugMode(bool enabled);
    static bool isDebugMode();

private:
    static QSharedPointer<QtMyBatisORM> s_orm;
    static bool s_debugMode;
    static bool s_initialized;
    
    // RAII Session manager
    class SessionScope;
    
    // Internal helper methods
    static void checkInitialized();
};

} // namespace QtMyBatisORM 