#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

// 定义一个简单的测试类，避免与test_multi_database_integration.cpp中的类冲突
class TestMultiDatabaseIntegrationSimple : public QObject
{
    Q_OBJECT

private slots:
    void testBasic();
};

void TestMultiDatabaseIntegrationSimple::testBasic()
{
    // 简单的测试，确保编译通过
    QVERIFY(true);
}

QTEST_MAIN(TestMultiDatabaseIntegrationSimple)
#include "run_multi_database_integration_test.moc"