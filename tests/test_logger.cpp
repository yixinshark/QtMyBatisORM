#include <QtTest/QtTest>
#include "QtMyBatisORM/logger.h"

using namespace QtMyBatisORM;

class TestLogger : public QObject
{
    Q_OBJECT

public:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testLogLevelSettings();
    void testLogLevels();
    void testLogWithContext();
    void testLogFormatting();
};

// TestLogger implementation - class definition should be in the header or main test file

void TestLogger::initTestCase()
{
    // 初始化测试环境
    Logger::setLogLevel(LogLevel::DEBUG_LEVEL);
}

void TestLogger::cleanupTestCase()
{
    // 清理测试环境
}

void TestLogger::init()
{
    // 每个测试方法之前的初始化
}

void TestLogger::cleanup()
{
    // 每个测试方法之后的清理
}

void TestLogger::testLogLevelSettings()
{
    // 测试设置和获取日志级别
    
    // 测试设置Info级别
    Logger::setLogLevel(LogLevel::INFO);
    QCOMPARE(Logger::getLogLevel(), LogLevel::INFO);
    
    // 测试设置Debug级别
    Logger::setLogLevel(LogLevel::DEBUG_LEVEL);
    QCOMPARE(Logger::getLogLevel(), LogLevel::DEBUG_LEVEL);
    
    // 测试设置Warning级别
    Logger::setLogLevel(LogLevel::WARN);
    QCOMPARE(Logger::getLogLevel(), LogLevel::WARN);
    
    // 测试设置Error级别
    Logger::setLogLevel(LogLevel::ERROR);
    QCOMPARE(Logger::getLogLevel(), LogLevel::ERROR);
}

void TestLogger::testLogLevels()
{
    // 重置为Debug级别以确保所有消息都会被记录
    Logger::setLogLevel(LogLevel::DEBUG_LEVEL);
    
    // 测试不同级别的日志记录
    Logger::debug(QStringLiteral("Debug message"));
    Logger::info(QStringLiteral("Info message"));
    Logger::warn(QStringLiteral("Warning message"));
    Logger::error(QStringLiteral("Error message"));
    
    // 这些调用不应该崩溃或抛出异常
    QVERIFY(true);
}

void TestLogger::testLogWithContext()
{
    // 测试带上下文信息的日志记录
    QVariantMap context;
    context[QStringLiteral("testContext")] = QStringLiteral("TestValue");
    
    Logger::debug(QStringLiteral("Debug with context"), context);
    Logger::info(QStringLiteral("Info with context"), context);
    Logger::warn(QStringLiteral("Warning with context"), context);
    Logger::error(QStringLiteral("Error with context"), context);
    
    // 验证不会崩溃
    QVERIFY(true);
}

void TestLogger::testLogFormatting()
{
    // 测试日志格式化功能
    QString message = QStringLiteral("Test message with parameters: %1, %2");
    QString param1 = QStringLiteral("param1");
    int param2 = 42;
    
    // 格式化消息
    QString formatted = message.arg(param1).arg(param2);
    Logger::info(formatted);
    
    // 测试空消息
    Logger::info(QString());
    
    // 测试特殊字符
    Logger::info(QStringLiteral("Special chars: !@#$%^&*()"));
    
    // 测试Unicode字符
    Logger::info(QStringLiteral("Unicode: 中文测试 🚀"));
    
    // 验证不会崩溃
    QVERIFY(true);
}

#include "test_logger.moc"
