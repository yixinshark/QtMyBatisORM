#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QSharedPointer>
#include <QTimer>
#include <QStack>
#include <QDateTime>
#include "qtmybatisexception.h"

namespace QtMyBatisORM {

class Executor;
class MapperRegistry;

/**
 * 数据库会话
 */
class Session : public QObject
{
    Q_OBJECT
    
public:
    explicit Session(QSharedPointer<QSqlDatabase> connection,
                    QSharedPointer<Executor> executor,
                    QSharedPointer<MapperRegistry> mapperRegistry,
                    QObject* parent = nullptr);
    
    // 基本CRUD操作
    QVariant selectOne(const QString& statementId, const QVariantMap& parameters = {});
    QVariantList selectList(const QString& statementId, const QVariantMap& parameters = {});
    int insert(const QString& statementId, const QVariantMap& parameters = {});
    int update(const QString& statementId, const QVariantMap& parameters = {});
    int remove(const QString& statementId, const QVariantMap& parameters = {});
    
    // 执行原始SQL语句
    int execute(const QString& sql, const QVariantMap& parameters = {});
    
    // 批量操作
    int batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList);
    int batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList);
    int batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList);
    
    // 事务管理
    void beginTransaction();
    void beginTransaction(int timeoutSeconds);
    void commit();
    void rollback();
    bool isInTransaction() const;
    
    // 嵌套事务支持 (保存点)
    QString setSavepoint(const QString& savepointName = QString());
    void rollbackToSavepoint(const QString& savepointName);
    void releaseSavepoint(const QString& savepointName);
    
    // 事务状态查询
    int getTransactionLevel() const;
    QDateTime getTransactionStartTime() const;
    bool isTransactionTimedOut() const;
    
    // 获取Mapper
    template<typename T>
    T* getMapper();
    
    void close();
    bool isClosed() const;
    
private slots:
    void onTransactionTimeout();
    
private:
    QString getStatementSql(const QString& statementId);
    void checkClosed();
    void checkTransactionTimeout();
    QString generateSavepointName();
    
    QSharedPointer<QSqlDatabase> m_connection;
    QSharedPointer<Executor> m_executor;
    QSharedPointer<MapperRegistry> m_mapperRegistry;
    
    bool m_autoCommit;
    bool m_inTransaction;
    bool m_closed;
    
    // 事务管理相关
    QDateTime m_transactionStartTime;
    QDateTime m_transactionTimeoutPoint; // 添加事务超时时间点
    int m_transactionTimeoutSeconds;
    QTimer* m_transactionTimer;
    
    // 嵌套事务支持
    QStack<QString> m_savepointStack;
    int m_savepointCounter;
};

} // namespace QtMyBatisORM