#ifndef QTMYBATISORM_DYNAMICSQLPROCESSOR_H
#define QTMYBATISORM_DYNAMICSQLPROCESSOR_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QRegularExpression>

namespace QtMyBatisORM {

/**
 * @brief 动态SQL处理器，用于处理MyBatis风格的动态SQL语句
 * 
 * 支持以下动态SQL元素：
 * - #{param} - 参数替换
 * - <if test="condition">content</if> - 条件判断
 * - <foreach collection="collection" item="item" separator="," open="(" close=")">content</foreach> - 循环
 * - <choose><when test="condition">content</when><otherwise>content</otherwise></choose> - 选择
 * - <where>content</where> - WHERE子句处理
 * - <set>content</set> - SET子句处理
 */
class DynamicSqlProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DynamicSqlProcessor(QObject* parent = nullptr);

    /**
     * @brief 处理动态SQL语句
     * @param sql 包含动态元素的SQL语句
     * @param parameters 参数映射
     * @return 处理后的SQL语句
     */
    QString process(const QString& sql, const QVariantMap& parameters);

private:
    /**
     * @brief 处理if条件
     * @param condition 条件表达式
     * @param content if内容
     * @param parameters 参数映射
     * @return 处理后的内容
     */
    QString processIf(const QString& condition, const QString& content, const QVariantMap& parameters);

    /**
     * @brief 处理foreach循环
     * @param collection 集合名称
     * @param item 项名称
     * @param content foreach内容
     * @param parameters 参数映射
     * @param separator 分隔符
     * @param open 开始字符
     * @param close 结束字符
     * @return 处理后的内容
     */
    QString processForeach(const QString& collection, const QString& item, 
                          const QString& content, const QVariantMap& parameters,
                          const QString& separator, const QString& open,
                          const QString& close);

    /**
     * @brief 处理choose/when/otherwise结构
     * @param content choose内容
     * @param parameters 参数映射
     * @return 处理后的内容
     */
    QString processChoose(const QString& content, const QVariantMap& parameters);

    /**
     * @brief 处理where子句
     * @param content where内容
     * @param parameters 参数映射
     * @return 处理后的内容
     */
    QString processWhere(const QString& content, const QVariantMap& parameters);

    /**
     * @brief 处理set子句
     * @param content set内容
     * @param parameters 参数映射
     * @return 处理后的内容
     */
    QString processSet(const QString& content, const QVariantMap& parameters);

    /**
     * @brief 评估条件表达式
     * @param condition 条件表达式
     * @param parameters 参数映射
     * @return 条件是否为真
     */
    bool evaluateCondition(const QString& condition, const QVariantMap& parameters);

    /**
     * @brief 替换参数占位符
     * @param content 包含参数占位符的内容
     * @param parameters 参数映射
     * @return 替换后的内容
     */
    QString replaceParameters(const QString& content, const QVariantMap& parameters);

private:
    QRegularExpression m_ifPattern;       ///< if标签的正则表达式
    QRegularExpression m_foreachPattern;  ///< foreach标签的正则表达式
    QRegularExpression m_choosePattern;   ///< choose标签的正则表达式
    QRegularExpression m_wherePattern;    ///< where标签的正则表达式
    QRegularExpression m_setPattern;      ///< set标签的正则表达式
};

} // namespace QtMyBatisORM

#endif // QTMYBATISORM_DYNAMICSQLPROCESSOR_H