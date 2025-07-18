#include "QtMyBatisORM/logger.h"
#include <QThread>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QStringBuilder>

namespace QtMyBatisORM {

// 静态成员初始化
LogLevel Logger::s_logLevel = LogLevel::INFO;
QMutex Logger::s_mutex;
QHash<LogLevel, QString> Logger::s_levelStrings;
QElapsedTimer Logger::s_startTime;
QDateTime Logger::s_startDateTime;
thread_local QString Logger::t_threadId;

// 初始化静态成员
class LoggerInitializer {
public:
    LoggerInitializer() {
        // 预先缓存日志级别字符串
        Logger::s_levelStrings[LogLevel::TRACE] = QStringLiteral("TRACE");
        Logger::s_levelStrings[LogLevel::DEBUG_LEVEL] = QStringLiteral("DEBUG");
        Logger::s_levelStrings[LogLevel::INFO]  = QStringLiteral("INFO ");
        Logger::s_levelStrings[LogLevel::WARN]  = QStringLiteral("WARN ");
        Logger::s_levelStrings[LogLevel::ERROR] = QStringLiteral("ERROR");
        Logger::s_levelStrings[LogLevel::FATAL] = QStringLiteral("FATAL");
        
        // 初始化计时器
        Logger::s_startTime.start();
        Logger::s_startDateTime = QDateTime::currentDateTime();
    }
};

// 确保在程序启动时初始化
static LoggerInitializer loggerInitializer;

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&s_mutex);
    s_logLevel = level;
}

LogLevel Logger::getLogLevel()
{
    // 读取日志级别不需要锁，因为它是原子操作
    return s_logLevel;
}

void Logger::trace(const QString& message, const QVariantMap& context)
{
    // 快速检查日志级别，避免不必要的函数调用
    if (s_logLevel <= LogLevel::TRACE) {
        log(LogLevel::TRACE, message, context);
    }
}

void Logger::debug(const QString& message, const QVariantMap& context)
{
    if (s_logLevel <= LogLevel::DEBUG_LEVEL) {
        log(LogLevel::DEBUG_LEVEL, message, context);
    }
}

void Logger::info(const QString& message, const QVariantMap& context)
{
    if (s_logLevel <= LogLevel::INFO) {
        log(LogLevel::INFO, message, context);
    }
}

void Logger::warn(const QString& message, const QVariantMap& context)
{
    if (s_logLevel <= LogLevel::WARN) {
        log(LogLevel::WARN, message, context);
    }
}

void Logger::error(const QString& message, const QVariantMap& context)
{
    if (s_logLevel <= LogLevel::ERROR) {
        log(LogLevel::ERROR, message, context);
    }
}

void Logger::fatal(const QString& message, const QVariantMap& context)
{
    if (s_logLevel <= LogLevel::FATAL) {
        log(LogLevel::FATAL, message, context);
    }
}

void Logger::log(LogLevel level, const QString& message, const QVariantMap& context)
{
    // 快速检查日志级别，避免不必要的格式化
    if (level < s_logLevel) {
        return;
    }
    
    QString formattedMessage = formatLogMessage(level, message, context);
    
    // 不需要锁定互斥量，因为Qt的日志系统是线程安全的
    switch (level) {
        case LogLevel::TRACE:
        case LogLevel::DEBUG_LEVEL:
            qDebug().noquote() << formattedMessage;
            break;
        case LogLevel::INFO:
            qInfo().noquote() << formattedMessage;
            break;
        case LogLevel::WARN:
            qWarning().noquote() << formattedMessage;
            break;
        case LogLevel::ERROR:
        case LogLevel::FATAL:
            qCritical().noquote() << formattedMessage;
            break;
    }
}

QString Logger::formatLogMessage(LogLevel level, const QString& message, const QVariantMap& context)
{
    // 使用线程本地存储缓存线程ID
    if (t_threadId.isEmpty()) {
        t_threadId = QString::number((quintptr)QThread::currentThreadId());
    }
    
    // 使用QStringBuilder (operator%) 进行高效字符串连接
    QString formattedMessage = QLatin1String("[") % getCurrentTimestamp() % 
                              QLatin1String("] [") % levelToString(level) % 
                              QLatin1String("] [Thread-") % t_threadId % 
                              QLatin1String("] ") % message;
    
    if (!context.isEmpty()) {
        // 使用更高效的方式格式化上下文，避免JSON序列化
        formattedMessage += QLatin1String(" | Context: {");
        
        bool first = true;
        for (auto it = context.constBegin(); it != context.constEnd(); ++it) {
            if (!first) {
                formattedMessage += QLatin1String(", ");
            }
            first = false;
            
            formattedMessage += it.key() % QLatin1String(": ");
            
            // 根据值类型进行高效格式化
            const QVariant& value = it.value();
            switch (value.typeId()) {
                case QMetaType::Int:
                    formattedMessage += QString::number(value.toInt());
                    break;
                case QMetaType::Double:
                    formattedMessage += QString::number(value.toDouble(), 'g', 6);
                    break;
                case QMetaType::Bool:
                    formattedMessage += value.toBool() ? QLatin1String("true") : QLatin1String("false");
                    break;
                case QMetaType::QString:
                    formattedMessage += QLatin1String("\"") % value.toString() % QLatin1String("\"");
                    break;
                default:
                    formattedMessage += QLatin1String("\"") % value.toString() % QLatin1String("\"");
                    break;
            }
        }
        
        formattedMessage += QLatin1String("}");
    }
    
    return formattedMessage;
}

QString Logger::levelToString(LogLevel level)
{
    // 使用预缓存的字符串，避免每次都创建新字符串
    return s_levelStrings.value(level, QLatin1String("UNKNW"));
}

QString Logger::getCurrentTimestamp()
{
    // 使用高效的时间戳生成方法
    // 基于程序启动时间和经过的毫秒数计算当前时间，避免频繁系统调用
    qint64 elapsed = s_startTime.elapsed();
    QDateTime now = s_startDateTime.addMSecs(elapsed);
    
    // 使用预分配的字符串缓冲区
    static thread_local QString buffer;
    if (buffer.isEmpty()) {
        buffer.reserve(23); // yyyy-MM-dd hh:mm:ss.zzz
    }
    
    return now.toString(QLatin1String("yyyy-MM-dd hh:mm:ss.zzz"));
}

} // namespace QtMyBatisORM