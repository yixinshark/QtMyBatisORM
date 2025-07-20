#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QtMyBatisORM/qtmybatishelper.h>
#include <QtMyBatisORM/qtmybatisexception.h>

// 定义一个简单的日志类
class ConsistencyLog {
public:
    int id;
    int dateTime;
    QString systemId;
    QString fileName;
    QString type;
    QString processMeans;
    
    ConsistencyLog() : id(0), dateTime(0) {}
    
    ConsistencyLog(int dateTime, const QString& systemId, const QString& fileName, 
                  const QString& type, const QString& processMeans)
        : id(0), dateTime(dateTime), systemId(systemId), fileName(fileName), 
          type(type), processMeans(processMeans) {}
    
    // 转换为QVariantMap，用于插入数据库
    QVariantMap toMap() const {
        QVariantMap map;
        if (id > 0) map["id"] = id;
        map["dateTime"] = dateTime;
        map["systemId"] = systemId;
        map["fileName"] = fileName;
        map["type"] = type;
        map["processMeans"] = processMeans;
        return map;
    }
    
    // 从QVariantMap创建对象，用于从数据库读取
    static ConsistencyLog fromMap(const QVariantMap& map) {
        ConsistencyLog log;
        log.id = map["id"].toInt();
        log.dateTime = map["dateTime"].toInt();
        log.systemId = map["systemId"].toString();
        log.fileName = map["fileName"].toString();
        log.type = map["type"].toString();
        log.processMeans = map["processMeans"].toString();
        return log;
    }
};

// 简化的DAO类 - 完全不需要知道ORM和Session
class ConsistencyLogDao 
{
public:
    // 创建表
    static bool createTable() {
        try {
            return QtMyBatisORM::QtMyBatisHelper::execute("ConsistencyLog.CreatTable") >= 0;
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "创建表失败:" << e.message();
            return false;
        }
    }
    
    // 基本查询
    static int getCount(int startTime, int endTime) {
        try {
            QVariantMap params;
            params["arg1"] = startTime;
            params["arg2"] = endTime;
            
            return QtMyBatisORM::QtMyBatisHelper::selectOne("ConsistencyLog.datasCount", params).toInt();
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "查询数量失败:" << e.message();
            return -1;
        }
    }
    
    static QList<ConsistencyLog> findAll() {
        try {
            QVariantList results = QtMyBatisORM::QtMyBatisHelper::selectList("ConsistencyLog.findAll");
            
            QList<ConsistencyLog> logs;
            for (const auto& result : results) {
                logs.append(ConsistencyLog::fromMap(result.toMap()));
            }
            return logs;
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "查询全部失败:" << e.message();
            return {};
        }
    }
    
    static QList<ConsistencyLog> findByDateTime(int startTime, int endTime, int offset, int limit) {
        try {
            QVariantMap params;
            params["arg1"] = startTime;
            params["arg2"] = endTime;
            params["arg3"] = offset;
            params["arg4"] = limit;
            
            QVariantList results = QtMyBatisORM::QtMyBatisHelper::selectList("ConsistencyLog.findByDateTime", params);
            
            QList<ConsistencyLog> logs;
            for (const auto& result : results) {
                logs.append(ConsistencyLog::fromMap(result.toMap()));
            }
            return logs;
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "分页查询失败:" << e.message();
            return {};
        }
    }
    
    // 增删改
    static bool insert(const ConsistencyLog& log) {
        try {
            return QtMyBatisORM::QtMyBatisHelper::insert("ConsistencyLog.insert", log.toMap()) > 0;
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "插入记录失败:" << e.message();
            return false;
        }
    }
    
    static int deleteByDateTime(int endTime) {
        try {
            QVariantMap params;
            params["arg1"] = endTime;
            return QtMyBatisORM::QtMyBatisHelper::remove("ConsistencyLog.deleteByDateTime", params);
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "删除记录失败:" << e.message();
            return -1;
        }
    }
    
    // 批量操作
    static bool insertBatch(const QList<ConsistencyLog>& logs) {
        try {
            QList<QVariantMap> paramsList;
            for (const auto& log : logs) {
                paramsList.append(log.toMap());
            }
            
            return QtMyBatisORM::QtMyBatisHelper::batchInsert("ConsistencyLog.insert", paramsList) == logs.size();
        } catch (const QtMyBatisORM::QtMyBatisException& e) {
            qWarning() << "批量插入失败:" << e.message();
            return false;
        }
    }
    
    // 复杂业务操作（带事务）
    static bool processLogsBatch(const QList<ConsistencyLog>& logs) {
        // 使用事务处理复杂业务逻辑
        return QtMyBatisORM::QtMyBatisHelper::executeInTransaction([&logs]() -> bool {
            try {
                // 先删除旧数据
                QVariantMap deleteParams;
                deleteParams["arg1"] = QDateTime::currentSecsSinceEpoch() - 86400; // 删除1天前的数据
                QtMyBatisORM::QtMyBatisHelper::remove("ConsistencyLog.deleteByDateTime", deleteParams);
                
                // 批量插入新数据
                for (const auto& log : logs) {
                    int result = QtMyBatisORM::QtMyBatisHelper::insert("ConsistencyLog.insert", log.toMap());
                    if (result <= 0) {
                        return false; // 任何插入失败都回滚
                    }
                }
                
                return true;
            } catch (...) {
                return false; // 异常时回滚
            }
        });
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Qt MyBatis ORM 新格式示例 ===";
    
    // 1. 一行代码初始化，自动读取配置和SQL文件
    if (!QtMyBatisORM::QtMyBatisHelper::initialize(":/config/database.json")) {
        qCritical() << "数据库初始化失败";
        return 1;
    }
    
    // 2. 开启调试模式（可选）
    QtMyBatisORM::QtMyBatisHelper::enableDebugMode(true);
    qDebug() << "调试模式已开启";
    
    try {
        // 3. 直接使用DAO，无需关心Session和连接
        qDebug() << "\n--- 创建表 ---";
        if (ConsistencyLogDao::createTable()) {
            qDebug() << "表创建成功";
        }
        
        // 4. 插入测试数据
        qDebug() << "\n--- 插入数据 ---";
        ConsistencyLog log1(QDateTime::currentSecsSinceEpoch(), "SYS001", "test1.log", "INFO", "AUTO");
        ConsistencyLog log2(QDateTime::currentSecsSinceEpoch() + 1, "SYS002", "test2.log", "ERROR", "MANUAL");
        ConsistencyLog log3(QDateTime::currentSecsSinceEpoch() + 2, "SYS001", "test3.log", "WARN", "AUTO");
        
        if (ConsistencyLogDao::insert(log1)) {
            qDebug() << "插入log1成功";
        }
        if (ConsistencyLogDao::insert(log2)) {
            qDebug() << "插入log2成功";
        }
        if (ConsistencyLogDao::insert(log3)) {
            qDebug() << "插入log3成功";
        }
        
        // 5. 查询数据
        qDebug() << "\n--- 查询数据 ---";
        auto allLogs = ConsistencyLogDao::findAll();
        qDebug() << "总共有" << allLogs.size() << "条记录";
        
        for (const auto& log : allLogs) {
            qDebug() << QString("记录: id=%1, system=%2, file=%3, type=%4")
                        .arg(log.id).arg(log.systemId).arg(log.fileName).arg(log.type);
        }
        
        // 6. 统计查询
        int count = ConsistencyLogDao::getCount(QDateTime::currentSecsSinceEpoch() - 100, 
                                               QDateTime::currentSecsSinceEpoch() + 100);
        qDebug() << "指定时间范围内的记录数:" << count;
        
        // 7. 分页查询
        qDebug() << "\n--- 分页查询 ---";
        auto pagedLogs = ConsistencyLogDao::findByDateTime(
            QDateTime::currentSecsSinceEpoch() - 100, 
            QDateTime::currentSecsSinceEpoch() + 100, 
            0, 2
        );
        qDebug() << "分页查询结果:" << pagedLogs.size() << "条记录";
        
        // 8. 批量操作（带事务）
        qDebug() << "\n--- 批量操作（事务） ---";
        QList<ConsistencyLog> batchLogs = {
            ConsistencyLog(QDateTime::currentSecsSinceEpoch() + 10, "BATCH1", "batch1.log", "INFO", "BATCH"),
            ConsistencyLog(QDateTime::currentSecsSinceEpoch() + 11, "BATCH2", "batch2.log", "INFO", "BATCH"),
            ConsistencyLog(QDateTime::currentSecsSinceEpoch() + 12, "BATCH3", "batch3.log", "INFO", "BATCH")
        };
        
        if (ConsistencyLogDao::processLogsBatch(batchLogs)) {
            qDebug() << "批量处理成功（包含删除旧数据和插入新数据）";
        }
        
        // 9. 验证最终结果
        qDebug() << "\n--- 最终统计 ---";
        auto finalLogs = ConsistencyLogDao::findAll();
        qDebug() << "最终记录数:" << finalLogs.size();
        
    } catch (const QtMyBatisORM::QtMyBatisException& e) {
        qCritical() << "操作失败:" << e.message();
        return 1;
    }
    
    // 10. 程序结束前清理
    QtMyBatisORM::QtMyBatisHelper::shutdown();
    qDebug() << "\n=== 示例执行完成 ===";
    
    return 0;
}

/*
新配置文件格式示例 (:/config/database.json):

{
    "database": {
        "debug": true,
        "type": "QSQLITE",
        "host": "127.0.0.1",
        "port": 3306,
        "database_name": ":memory:",
        "username": "test",
        "password": "11111111",
        "max_wait_time": 5000,
        "max_connection_count": 5,
        "sql_files": [
            ":/sql/consistencylog.sql"
        ]
    }
}

SQL文件示例 (:/sql/consistencylog.sql):

<?xml version="1.0" encoding="UTF-8"?>
<sqls namespace="ConsistencyLog">
    <define id="fields">id, dateTime, systemId, fileName, type, processMeans</define>

    <sql id="CreatTable">
        CREATE TABLE IF not EXISTS `consistencylog` (
        `id` INTEGER PRIMARY KEY AUTOINCREMENT,
        `dateTime` INTEGER NOT NULL,
        `systemId` VARCHAR(255) NOT NULL,
        `fileName` VARCHAR(255) NOT NULL,
        `type` VARCHAR(255) DEFAULT NULL,
        `processMeans` VARCHAR(255) DEFAULT NULL
        )
    </sql>

    <sql id="datasCount">
        SELECT count(*) FROM consistencylog WHERE dateTime >= %1 and dateTime <= %2
    </sql>

    <sql id="findAll">
        SELECT <include defineId="fields"/> FROM consistencylog
    </sql>
	
    <sql id="findByDateTime">
        SELECT <include defineId="fields"/> FROM consistencylog WHERE dateTime >= %1 and dateTime <= %2 order by dateTime limit %3, %4
    </sql>

    <sql id="insert">
        INSERT INTO consistencylog (dateTime, systemId, fileName, type, processMeans)
        VALUES (:dateTime, :systemId, :fileName, :type, :processMeans)
    </sql>

    <sql id="deleteByDateTime">
        DELETE FROM consistencylog WHERE dateTime <= %1
    </sql>
</sqls>
*/ 