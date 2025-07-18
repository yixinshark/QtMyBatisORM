#include "QtMyBatisORM/statementhandler.h"
#include "QtMyBatisORM/dynamicsqlprocessor.h"
#include <QSqlQuery>
#include <QRegularExpression>

namespace QtMyBatisORM {

StatementHandler::StatementHandler(QObject* parent)
    : QObject(parent)
{
    m_dynamicProcessor = new DynamicSqlProcessor(this);
}

QSqlQuery StatementHandler::prepare(const QString& sql, QSqlDatabase& db)
{
    QSqlQuery query(db);
    query.prepare(sql);
    return query;
}

void StatementHandler::setParameters(QSqlQuery& query, const QVariantMap& parameters)
{
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        bindParameter(query, it.key(), it.value());
    }
}

QString StatementHandler::processSql(const QString& sql, const QVariantMap& parameters)
{
    // 首先处理动态SQL
    QString processedSql = m_dynamicProcessor->process(sql, parameters);
    
    // 然后处理参数占位符
    return processedSql;
}

void StatementHandler::bindParameter(QSqlQuery& query, const QString& placeholder, const QVariant& value)
{
    // 支持命名参数和位置参数
    if (placeholder.startsWith(QStringLiteral(":"))) {
        query.bindValue(placeholder, value);
    } else {
        // 尝试作为位置参数
        bool ok;
        int pos = placeholder.toInt(&ok);
        if (ok) {
            query.bindValue(pos, value);
        } else {
            // 作为命名参数处理
            query.bindValue(":" + placeholder, value);
        }
    }
}

QStringList StatementHandler::extractPlaceholders(const QString& sql)
{
    QStringList placeholders;
    QRegularExpression re(QStringLiteral(R"(:(\w+))"));
    QRegularExpressionMatchIterator it = re.globalMatch(sql);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        placeholders.append(match.captured(1));
    }
    
    return placeholders;
}

} // namespace QtMyBatisORM