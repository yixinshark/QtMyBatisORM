#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QDebug>

#include "QtMyBatisORM/logger.h"

using namespace QtMyBatisORM;

class TestPerformanceBenchmark : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 简化的性能测试
    void testLoggerPerformance();
    void testBasicPerformance();

private:
    QElapsedTimer m_timer;
    void reportPerformance(const QString& testName, qint64 elapsedMs, int iterations);
};

void TestPerformanceBenchmark::initTestCase()
{
    // 设置日志级别
    Logger::setLogLevel(LogLevel::ERROR);
}

void TestPerformanceBenchmark::cleanupTestCase()
{
    // 清理
}

void TestPerformanceBenchmark::testLoggerPerformance()
{
    const int iterations = 1000;
    
    m_timer.start();
    for (int i = 0; i < iterations; ++i) {
        Logger::debug(QStringLiteral("Performance test message %1").arg(i));
    }
    qint64 elapsed = m_timer.elapsed();
    
    reportPerformance(QStringLiteral("Logger Performance"), elapsed, iterations);
    
    // 验证性能在合理范围内 (1000条消息应该在100ms内完成)
    QVERIFY(elapsed < 100);
}

void TestPerformanceBenchmark::testBasicPerformance()
{
    const int iterations = 10000;
    
    m_timer.start();
    int sum = 0;
    for (int i = 0; i < iterations; ++i) {
        sum += i;
    }
    qint64 elapsed = m_timer.elapsed();
    
    reportPerformance(QStringLiteral("Basic Loop Performance"), elapsed, iterations);
    
    // 基本性能测试
    QVERIFY(elapsed < 10);
    QVERIFY(sum > 0);
}

void TestPerformanceBenchmark::reportPerformance(const QString& testName, qint64 elapsedMs, int iterations)
{
    qDebug() << "Performance:" << testName 
             << "took" << elapsedMs << "ms for" << iterations << "iterations"
             << "(" << (double(elapsedMs) / iterations) << "ms per operation)";
}

int main(int argc, char *argv[])
{
    TestPerformanceBenchmark test;
    return QTest::qExec(&test, argc, argv);
}

#include "run_performance_benchmark_test.moc"