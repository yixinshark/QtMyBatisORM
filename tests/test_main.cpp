#include <QtTest>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    int result = 0;
    
    qDebug() << "QtMyBatisORM Tests - Build successful!";
    qDebug() << "Run individual test files separately to execute tests.";
    
    return result;
}