#include "QtMyBatisORM/dynamicsqlprocessor.h"
#include <QRegularExpression>
#include <QStringList>
#include <QMetaType>

namespace QtMyBatisORM {

DynamicSqlProcessor::DynamicSqlProcessor(QObject* parent)
    : QObject(parent)
{
    // 初始化正则表达式
    m_ifPattern = QRegularExpression(QStringLiteral(R"(<if\s+test\s*=\s*['"](.*?)['"]>(.*?)</if>)"), 
                                    QRegularExpression::DotMatchesEverythingOption);
    m_foreachPattern = QRegularExpression(QStringLiteral(R"(<foreach\s+collection\s*=\s*['"](.*?)['"].*?>(.*?)</foreach>)"), 
                                         QRegularExpression::DotMatchesEverythingOption);
    m_choosePattern = QRegularExpression(QStringLiteral(R"(<choose>(.*?)</choose>)"), 
                                        QRegularExpression::DotMatchesEverythingOption);
    m_wherePattern = QRegularExpression(QStringLiteral(R"(<where>(.*?)</where>)"), 
                                       QRegularExpression::DotMatchesEverythingOption);
    m_setPattern = QRegularExpression(QStringLiteral(R"(<set>(.*?)</set>)"), 
                                     QRegularExpression::DotMatchesEverythingOption);
}

QString DynamicSqlProcessor::process(const QString& sql, const QVariantMap& parameters)
{
    QString result = sql;
    
    // 处理foreach循环 - 需要先处理，因为可能包含其他动态元素
    QRegularExpressionMatchIterator foreachIterator = m_foreachPattern.globalMatch(result);
    QList<QRegularExpressionMatch> foreachMatches;
    while (foreachIterator.hasNext()) {
        foreachMatches.append(foreachIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = foreachMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = foreachMatches[i];
        QString fullMatch = match.captured(0);
        QString collection = match.captured(1);
        QString content = match.captured(2);
        
        // 手动解析foreach的属性
        QString item = QStringLiteral("item");
        QString separator = QStringLiteral(",");
        QString open = QStringLiteral("");
        QString close = QStringLiteral("");
        
        // 解析item属性
        QRegularExpression itemPattern(QStringLiteral(R"(item\s*=\s*['"](.*?)['"])"));
        QRegularExpressionMatch itemMatch = itemPattern.match(fullMatch);
        if (itemMatch.hasMatch()) {
            item = itemMatch.captured(1);
        }
        
        // 解析separator属性
        QRegularExpression separatorPattern(QStringLiteral(R"(separator\s*=\s*['"](.*?)['"])"));
        QRegularExpressionMatch separatorMatch = separatorPattern.match(fullMatch);
        if (separatorMatch.hasMatch()) {
            separator = separatorMatch.captured(1);
        }
        
        // 解析open属性
        QRegularExpression openPattern(QStringLiteral(R"(open\s*=\s*['"](.*?)['"])"));
        QRegularExpressionMatch openMatch = openPattern.match(fullMatch);
        if (openMatch.hasMatch()) {
            open = openMatch.captured(1);
        }
        
        // 解析close属性
        QRegularExpression closePattern(QStringLiteral(R"(close\s*=\s*['"](.*?)['"])"));
        QRegularExpressionMatch closeMatch = closePattern.match(fullMatch);
        if (closeMatch.hasMatch()) {
            close = closeMatch.captured(1);
        }
        

        
        QString processed = processForeach(collection, item, content, parameters, separator, open, close);
        result.replace(match.captured(0), processed);
    }
    
    // 处理if条件
    QRegularExpressionMatchIterator ifIterator = m_ifPattern.globalMatch(result);
    QList<QRegularExpressionMatch> ifMatches;
    while (ifIterator.hasNext()) {
        ifMatches.append(ifIterator.next());
    }
    for (int i = ifMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = ifMatches[i];
        QString condition = match.captured(1);
        QString content = match.captured(2);
        QString processed = processIf(condition, content, parameters);
        result.replace(match.captured(0), processed);
    }
    
    // 处理choose/when/otherwise
    QRegularExpressionMatchIterator chooseIterator = m_choosePattern.globalMatch(result);
    QList<QRegularExpressionMatch> chooseMatches;
    while (chooseIterator.hasNext()) {
        chooseMatches.append(chooseIterator.next());
    }
    for (int i = chooseMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = chooseMatches[i];
        QString content = match.captured(1);
        QString processed = processChoose(content, parameters);
        result.replace(match.captured(0), processed);
    }
    
    // 处理where子句
    QRegularExpressionMatchIterator whereIterator = m_wherePattern.globalMatch(result);
    QList<QRegularExpressionMatch> whereMatches;
    while (whereIterator.hasNext()) {
        whereMatches.append(whereIterator.next());
    }
    for (int i = whereMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = whereMatches[i];
        QString content = match.captured(1);
        QString processed = processWhere(content, parameters);
        result.replace(match.captured(0), processed);
    }
    
    // 处理set子句
    QRegularExpressionMatchIterator setIterator = m_setPattern.globalMatch(result);
    QList<QRegularExpressionMatch> setMatches;
    while (setIterator.hasNext()) {
        setMatches.append(setIterator.next());
    }
    for (int i = setMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = setMatches[i];
        QString content = match.captured(1);
        QString processed = processSet(content, parameters);
        result.replace(match.captured(0), processed);
    }
    
    // 替换参数占位符
    result = replaceParameters(result, parameters);
    
    return result.trimmed();
}

QString DynamicSqlProcessor::processIf(const QString& condition, const QString& content, const QVariantMap& parameters)
{
    if (evaluateCondition(condition, parameters)) {
        return content;
    }
    return QString();
}

QString DynamicSqlProcessor::processForeach(const QString& collection, const QString& item, 
                                           const QString& content, const QVariantMap& parameters,
                                           const QString& separator, const QString& open,
                                           const QString& close)
{
    Q_UNUSED(item); // 标记参数为已使用，避免警告
    if (!parameters.contains(collection)) {
        return QString();
    }
    
    QVariant collectionValue = parameters[collection];
    if (collectionValue.typeId() == QMetaType::QVariantList) {
        QVariantList list = collectionValue.toList();
        if (list.isEmpty()) {
            return QString();
        }
        
        QStringList results;
        for (int i = 0; i < list.size(); ++i) {
            QString itemContent = content;
            // 不在这里替换参数，让后续的replaceParameters统一处理
            results.append(itemContent);
        }
        
        QString result = results.join(separator);
        return open + result + close;
    }
    
    return QString();
}

QString DynamicSqlProcessor::processChoose(const QString& content, const QVariantMap& parameters)
{
    // 解析choose/when/otherwise结构
    QRegularExpression whenPattern(QStringLiteral(R"(<when\s+test\s*=\s*['"](.*?)['"]>(.*?)</when>)"), 
                                  QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression otherwisePattern(QStringLiteral(R"(<otherwise>(.*?)</otherwise>)"), 
                                       QRegularExpression::DotMatchesEverythingOption);
    
    // 查找所有when条件
    QRegularExpressionMatchIterator whenIterator = whenPattern.globalMatch(content);
    while (whenIterator.hasNext()) {
        QRegularExpressionMatch match = whenIterator.next();
        QString condition = match.captured(1);
        QString whenContent = match.captured(2);
        
        if (evaluateCondition(condition, parameters)) {
            return whenContent;
        }
    }
    
    // 如果没有when条件匹配，查找otherwise
    QRegularExpressionMatch otherwiseMatch = otherwisePattern.match(content);
    if (otherwiseMatch.hasMatch()) {
        return otherwiseMatch.captured(1);
    }
    
    return QString();
}

QString DynamicSqlProcessor::processWhere(const QString& content, const QVariantMap& parameters)
{
    QString processed = process(content, parameters);
    if (processed.isEmpty()) {
        return QString();
    }
    
    // 移除开头的AND或OR，并确保正确的空格处理
    processed = processed.trimmed();
    if (processed.startsWith(QStringLiteral("AND "), Qt::CaseInsensitive)) {
        processed = processed.mid(4).trimmed();
    } else if (processed.startsWith(QStringLiteral("OR "), Qt::CaseInsensitive)) {
        processed = processed.mid(3).trimmed();
    }
    
    return processed.isEmpty() ? QString() : "WHERE " + processed;
}

QString DynamicSqlProcessor::processSet(const QString& content, const QVariantMap& parameters)
{
    QString processed = process(content, parameters);
    if (processed.isEmpty()) {
        return QString();
    }
    
    // 移除末尾的逗号，并确保正确的空格处理
    processed = processed.trimmed();
    if (processed.endsWith(QStringLiteral(","))) {
        processed.chop(1);
        processed = processed.trimmed();
    }
    
    return processed.isEmpty() ? QString() : "SET " + processed;
}

bool DynamicSqlProcessor::evaluateCondition(const QString& condition, const QVariantMap& parameters)
{
    // 简单的条件评估实现
    QString trimmedCondition = condition.trimmed();
    
    // 处理 != null 条件
    if (trimmedCondition.contains(QStringLiteral(" != null"))) {
        QString paramName = trimmedCondition.split(QStringLiteral(" != null")).first().trimmed();
        return parameters.contains(paramName) && !parameters[paramName].isNull();
    }
    
    // 处理 == null 条件
    if (trimmedCondition.contains(QStringLiteral(" == null"))) {
        QString paramName = trimmedCondition.split(QStringLiteral(" == null")).first().trimmed();
        return !parameters.contains(paramName) || parameters[paramName].isNull();
    }
    
    // 处理简单的存在性检查
    if (parameters.contains(trimmedCondition)) {
        QVariant value = parameters[trimmedCondition];
        return !value.isNull() && value.isValid();
    }
    
    return false;
}

QString DynamicSqlProcessor::replaceParameters(const QString& content, const QVariantMap& parameters)
{
    QString result = content;
    
    // 替换 #{param} 格式的参数
    QRegularExpression paramPattern(QStringLiteral(R"(#\{(\w+)\})"));
    QRegularExpressionMatchIterator iterator = paramPattern.globalMatch(result);
    
    // 收集所有匹配项，然后从后往前替换
    QList<QRegularExpressionMatch> matches;
    while (iterator.hasNext()) {
        matches.append(iterator.next());
    }
    
    // 从后往前替换，避免位置偏移
    for (int i = matches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = matches[i];
        QString paramName = match.captured(1);
        if (parameters.contains(paramName)) {
            result.replace(match.captured(0), ":" + paramName);
        }
    }
    
    return result;
}

} // namespace QtMyBatisORM