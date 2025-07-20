#include <QCoreApplication>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QFile sqlFile(":/sql/init.sql");
    if (sqlFile.open(QIODevice::ReadOnly)) {
        QByteArray content = sqlFile.readAll();
        qDebug() << "SQL文件大小:" << content.size();
        qDebug() << "前100字符:" << content.left(100);
        qDebug() << "是否为有效UTF-8:" << content.toStdString().c_str();
        
        // 尝试作为文本读取
        QString text = QString::fromUtf8(content);
        qDebug() << "作为文本的前200字符:" << text.left(200);
    } else {
        qDebug() << "无法打开SQL文件";
    }
    
    return 0;
} 