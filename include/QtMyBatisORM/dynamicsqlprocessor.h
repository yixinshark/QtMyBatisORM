#ifndef QTMYBATISORM_DYNAMICSQLPROCESSOR_H
#define QTMYBATISORM_DYNAMICSQLPROCESSOR_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QRegularExpression>

namespace QtMyBatisORM {

/**
 * @brief Dynamic SQL processor for handling MyBatis-style dynamic SQL statements
 * 
 * Supports the following dynamic SQL elements:
 * - #{param} - Parameter substitution
 * - <if test="condition">content</if> - Conditional judgment
 * - <foreach collection="collection" item="item" separator="," open="(" close=")">content</foreach> - Loop
 * - <choose><when test="condition">content</when><otherwise>content</otherwise></choose> - Choice
 * - <where>content</where> - WHERE clause handling
 * - <set>content</set> - SET clause handling
 */
class DynamicSqlProcessor : public QObject
{
    Q_OBJECT

public:
    explicit DynamicSqlProcessor(QObject* parent = nullptr);

    /**
     * @brief Process dynamic SQL statements
     * @param sql SQL statement containing dynamic elements
     * @param parameters Parameter mapping
     * @return Processed SQL statement
     */
    QString process(const QString& sql, const QVariantMap& parameters);

private:
    /**
     * @brief Process if conditions
     * @param condition Condition expression
     * @param content If content
     * @param parameters Parameter mapping
     * @return Processed content
     */
    QString processIf(const QString& condition, const QString& content, const QVariantMap& parameters);

    /**
     * @brief Process foreach loops
     * @param collection Collection name
     * @param item Item name
     * @param content Foreach content
     * @param parameters Parameter mapping
     * @param separator Separator
     * @param open Opening character
     * @param close Closing character
     * @return Processed content
     */
    QString processForeach(const QString& collection, const QString& item, 
                          const QString& content, const QVariantMap& parameters,
                          const QString& separator, const QString& open,
                          const QString& close);

    /**
     * @brief Process choose/when/otherwise structure
     * @param content Choose content
     * @param parameters Parameter mapping
     * @return Processed content
     */
    QString processChoose(const QString& content, const QVariantMap& parameters);

    /**
     * @brief Process where clause
     * @param content Where content
     * @param parameters Parameter mapping
     * @return Processed content
     */
    QString processWhere(const QString& content, const QVariantMap& parameters);

    /**
     * @brief Process set clause
     * @param content Set content
     * @param parameters Parameter mapping
     * @return Processed content
     */
    QString processSet(const QString& content, const QVariantMap& parameters);

    /**
     * @brief Evaluate condition expression
     * @param condition Condition expression
     * @param parameters Parameter mapping
     * @return Whether condition is true
     */
    bool evaluateCondition(const QString& condition, const QVariantMap& parameters);

    /**
     * @brief Replace parameter placeholders
     * @param content Content containing parameter placeholders
     * @param parameters Parameter mapping
     * @return Content after replacement
     */
    QString replaceParameters(const QString& content, const QVariantMap& parameters);

private:
    QRegularExpression m_ifPattern;       ///< Regular expression for if tags
    QRegularExpression m_foreachPattern;  ///< Regular expression for foreach tags
    QRegularExpression m_choosePattern;   ///< Regular expression for choose tags
    QRegularExpression m_wherePattern;    ///< Regular expression for where tags
    QRegularExpression m_setPattern;      ///< Regular expression for set tags
};

} // namespace QtMyBatisORM

#endif // QTMYBATISORM_DYNAMICSQLPROCESSOR_H