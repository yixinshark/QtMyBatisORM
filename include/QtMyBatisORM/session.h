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

namespace QtMyBatisORM {

class Executor;
class MapperRegistry;

/**
 * Database session
 */
class Session : public QObject
{
    Q_OBJECT
    
public:
    explicit Session(QSharedPointer<QSqlDatabase> connection,
                    QSharedPointer<Executor> executor,
                    QSharedPointer<MapperRegistry> mapperRegistry,
                    QObject* parent = nullptr);
    
    // Basic CRUD operations
    QVariant selectOne(const QString& statementId, const QVariantMap& parameters = {});
    QVariantList selectList(const QString& statementId, const QVariantMap& parameters = {});
    int insert(const QString& statementId, const QVariantMap& parameters = {});
    int update(const QString& statementId, const QVariantMap& parameters = {});
    int remove(const QString& statementId, const QVariantMap& parameters = {});
    
    // Execute raw SQL statements
    int execute(const QString& sql, const QVariantMap& parameters = {});
    
    // Batch operations
    int batchInsert(const QString& statementId, const QList<QVariantMap>& parametersList);
    int batchUpdate(const QString& statementId, const QList<QVariantMap>& parametersList);
    int batchRemove(const QString& statementId, const QList<QVariantMap>& parametersList);
    
    // Transaction management
    void beginTransaction();
    void beginTransaction(int timeoutSeconds);
    void commit();
    void rollback();
    bool isInTransaction() const;
    
    // Nested transaction support (savepoints)
    // 嵌套事务支持 (保存点)
    QString setSavepoint(const QString& savepointName = QString());
    void rollbackToSavepoint(const QString& savepointName);
    void releaseSavepoint(const QString& savepointName);
    
    // Transaction status queries
    int getTransactionLevel() const;
    QDateTime getTransactionStartTime() const;
    bool isTransactionTimedOut() const;
    
    // Get Mapper
    template<typename T>
    T* getMapper();
    
    void close();
    bool isClosed() const;
    
    // Debug functionality
    void setDebugMode(bool enabled);
    bool isDebugMode() const;
    
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
    
    // Transaction management related
    QDateTime m_transactionStartTime;
    QDateTime m_transactionTimeoutPoint; // Add transaction timeout point
    int m_transactionTimeoutSeconds;
    QTimer* m_transactionTimer;
    
    // Nested transaction support
    // 嵌套事务支持
    QStack<QString> m_savepointStack;
    int m_savepointCounter;
};

} // namespace QtMyBatisORM