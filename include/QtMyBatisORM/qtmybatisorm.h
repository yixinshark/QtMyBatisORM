#pragma once

// Export macros for Windows/Unix compatibility
#if defined(QtMyBatisORM_EXPORTS)
#  define QTMYBATISORM_EXPORT Q_DECL_EXPORT
#else
#  define QTMYBATISORM_EXPORT Q_DECL_IMPORT
#endif

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include "sessionfactory.h"
#include "session.h"
#include "datamodels.h"

namespace QtMyBatisORM {

class SessionFactory;
class Session;

/**
 * Qt-MyBatis-ORM主入口类
 */
class QtMyBatisORM : public QObject
{
    Q_OBJECT
    
public:
    explicit QtMyBatisORM(QObject* parent = nullptr);
    ~QtMyBatisORM();
    
    // 初始化方法
    bool initialize(const QString& configPath, const QStringList& mapperPaths = {});
    bool initializeWithConfig(const DatabaseConfig& config, const QStringList& mapperPaths = {});
    
    // 获取会话工厂
    QSharedPointer<SessionFactory> getSessionFactory();
    
    // 便捷方法
    QSharedPointer<Session> openSession();
    void closeSession(QSharedPointer<Session> session);
    
    // 状态查询
    bool isInitialized() const;
    DatabaseConfig getDatabaseConfig() const;
    
    // 清理
    void shutdown();
    
    // 静态便捷方法
    static QSharedPointer<QtMyBatisORM> create(const QString& configPath, 
                                              const QStringList& mapperPaths = {});
    static QSharedPointer<QtMyBatisORM> createWithConfig(const DatabaseConfig& config,
                                                        const QStringList& mapperPaths = {});
    
    // 新增：一站式初始化方法，从资源文件自动加载配置和SQL文件
    static QSharedPointer<QtMyBatisORM> createFromResource(const QString& configResourcePath);
    
    // 快速创建方法 - 使用默认配置
    static QSharedPointer<QtMyBatisORM> createDefault();
    static QSharedPointer<QtMyBatisORM> createSQLite(const QString& databasePath = QLatin1String(":memory:"));
    static QSharedPointer<QtMyBatisORM> createMySQL(const QString& host, int port, 
                                                   const QString& database, 
                                                   const QString& username, 
                                                   const QString& password);
    
    // 便捷的SessionFactory创建方法
    static QSharedPointer<SessionFactory> createSessionFactory(const QString& configPath,
                                                              const QStringList& mapperPaths = {});
    static QSharedPointer<SessionFactory> createSessionFactoryWithConfig(const DatabaseConfig& config,
                                                                        const QStringList& mapperPaths = {});
    
private:
    void loadConfiguration(const QString& configPath);
    void loadMappers(const QStringList& mapperPaths);
    
    QSharedPointer<SessionFactory> m_sessionFactory;
    DatabaseConfig m_config;
    bool m_initialized;
};

} // namespace QtMyBatisORM