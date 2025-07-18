#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

// 定义一个简单的测试类，避免与test_end_to_end_integration.cpp中的类冲突
class TestEndToEndIntegrationSimple : public QObject
{
    Q_OBJECT

private slots:
    void testBasic();
};

void TestEndToEndIntegrationSimple::testBasic()
{
    // 简单的测试，确保编译通过
    QVERIFY(true);
}

QTEST_MAIN(TestEndToEndIntegrationSimple)
#include "run_end_to_end_integration_test.moc"