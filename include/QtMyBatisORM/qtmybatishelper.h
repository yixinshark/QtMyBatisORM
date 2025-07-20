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
 * 静态数据库操作工具类
 * 完全封装ORM和Session管理，提供简洁的静态接口
 */
class QTMYBATISORM_EXPORT QtMyBatisHelper
{
public:
    // 初始化方法
    static bool initialize(const QString& configResourcePath);
    static void shutdown();
    static bool isInitialized();
    
    // 基础CRUD操作 - 内部自动管理Session生命周期
    static QVariant selectOne(const QString& statementId, const QVariantMap& parameters = {});
    static QVariantList selectList(const QString& statementId, const QVariantMap& parameters = {});
    static int insert(const QString& statementId, const QVariantMap& parameters = {});
    static int update(const QString& statementId, const QVariantMap& parameters = {});
    static int remove(const QString& statementId, const QVariantMap& parameters = {});
    static int execute(const QString& sql, const QVariantMap& parameters = {});
    
    // 批量操作
    static int batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList);
    static int batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList);
    static int batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList);
    
    // 事务操作 - 确保Session正确关闭
    static bool executeInTransaction(std::function<bool()> operation);
    static bool executeInTransaction(std::function<bool(QSharedPointer<Session>)> operation);
    
    // 调试和监控
    static void enableDebugMode(bool enabled = true);
    static bool isDebugMode();

private:
    static QSharedPointer<QtMyBatisORM> s_orm;
    static bool s_debugMode;
    static bool s_initialized;
    
    // RAII Session管理器
    class SessionScope;
    
    // 内部辅助方法
    static void checkInitialized();
    static void logDebug(const QString& operation, const QString& statementId, 
                        const QVariantMap& parameters, qint64 elapsedMs, 
                        const QVariant& result = QVariant());
};

} // namespace QtMyBatisORM 