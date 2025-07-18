#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QMutex>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QElapsedTimer>
#include "export.h"

namespace QtMyBatisORM {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    TRACE,      ///< 最详细的跟踪信息
    DEBUG_LEVEL,///< 调试信息
    INFO,       ///< 一般信息
    WARN,       ///< 警告信息
    ERROR,      ///< 错误信息
    FATAL       ///< 致命错误
};

// 前向声明 LoggerInitializer 类
class LoggerInitializer;

/**
 * @brief 日志记录器类
 * 
 * 提供高性能结构化日志记录功能，支持不同日志级别和上下文信息
 */
class QTMYBATISORM_EXPORT Logger
{
    // 声明 LoggerInitializer 为友元类，使其可以访问 Logger 的私有成员
    friend class LoggerInitializer;
public:
    /**
     * @brief 设置全局日志级别
     * @param level 日志级别
     */
    static void setLogLevel(LogLevel level);
    
    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    static LogLevel getLogLevel();
    
    /**
     * @brief 记录跟踪级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void trace(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录调试级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void debug(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录信息级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void info(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录警告级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void warn(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录错误级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void error(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录致命错误级别日志
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void fatal(const QString& message, const QVariantMap& context = QVariantMap());
    
    /**
     * @brief 记录指定级别的日志
     * @param level 日志级别
     * @param message 日志消息
     * @param context 上下文信息（可选）
     */
    static void log(LogLevel level, const QString& message, const QVariantMap& context = QVariantMap());
    
private:
    static LogLevel s_logLevel;
    static QMutex s_mutex;
    static QHash<LogLevel, QString> s_levelStrings;
    static QElapsedTimer s_startTime;
    static QDateTime s_startDateTime;
    static thread_local QString t_threadId;
    
    /**
     * @brief 格式化日志消息
     * @param level 日志级别
     * @param message 日志消息
     * @param context 上下文信息
     * @return 格式化后的日志消息
     */
    static QString formatLogMessage(LogLevel level, const QString& message, const QVariantMap& context);
    
    /**
     * @brief 将日志级别转换为字符串
     * @param level 日志级别
     * @return 日志级别字符串
     */
    static QString levelToString(LogLevel level);
    
    /**
     * @brief 获取当前时间戳字符串
     * @return 格式化的时间戳字符串
     */
    static QString getCurrentTimestamp();
};

} // namespace QtMyBatisORM