#include <QtTest/QtTest>
#include <QVariantMap>
#include <QVariantList>
#include "QtMyBatisORM/dynamicsqlprocessor.h"

class TestDynamicSqlProcessor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testBasicParameterReplacement();
    void testIfCondition();
    void testForeachLoop();
    void testChooseWhenOtherwise();
    void testWhereClause();
    void testSetClause();

private:
    QtMyBatisORM::DynamicSqlProcessor* processor;
};

void TestDynamicSqlProcessor::initTestCase()
{
    processor = new QtMyBatisORM::DynamicSqlProcessor(this);
}

void TestDynamicSqlProcessor::cleanupTestCase()
{
    // processor will be deleted automatically as it's a child of this
}

void TestDynamicSqlProcessor::testBasicParameterReplacement()
{
    QString sql = "SELECT * FROM users WHERE id = #{userId} AND name = #{userName}";
    QVariantMap params;
    params["userId"] = 123;
    params["userName"] = "John";
    
    QString result = processor->process(sql, params);
    QString expected = "SELECT * FROM users WHERE id = :userId AND name = :userName";
    
    QCOMPARE(result, expected);
}

void TestDynamicSqlProcessor::testIfCondition()
{
    QString sql = "SELECT * FROM users WHERE 1=1 <if test=\"name\">AND name = #{name}</if>";
    
    // Test with name parameter
    QVariantMap params1;
    params1["name"] = "John";
    QString result1 = processor->process(sql, params1);
    QString expected1 = "SELECT * FROM users WHERE 1=1 AND name = :name";
    QCOMPARE(result1, expected1);
    
    // Test without name parameter
    QVariantMap params2;
    QString result2 = processor->process(sql, params2);
    QString expected2 = "SELECT * FROM users WHERE 1=1";
    QCOMPARE(result2, expected2);
}

void TestDynamicSqlProcessor::testForeachLoop()
{
    QString sql = "SELECT * FROM users WHERE id IN <foreach collection=\"ids\" item=\"id\" open=\"(\" close=\")\" separator=\",\">#{id}</foreach>";
    
    QVariantMap params;
    QVariantList ids;
    ids << 1 << 2 << 3;
    params["ids"] = ids;
    // 为了让参数替换工作，我们需要添加id参数
    params["id"] = "placeholder";
    
    QString result = processor->process(sql, params);
    QString expected = "SELECT * FROM users WHERE id IN (:id,:id,:id)";
    QCOMPARE(result, expected);
}

void TestDynamicSqlProcessor::testChooseWhenOtherwise()
{
    QString sql = "SELECT * FROM users <choose><when test=\"name\">WHERE name = #{name}</when><otherwise>WHERE status = 'active'</otherwise></choose>";
    
    // Test when condition is true
    QVariantMap params1;
    params1["name"] = "John";
    QString result1 = processor->process(sql, params1);
    QString expected1 = "SELECT * FROM users WHERE name = :name";
    QCOMPARE(result1, expected1);
    
    // Test when condition is false (otherwise case)
    QVariantMap params2;
    QString result2 = processor->process(sql, params2);
    QString expected2 = "SELECT * FROM users WHERE status = 'active'";
    QCOMPARE(result2, expected2);
}

void TestDynamicSqlProcessor::testWhereClause()
{
    QString sql = "SELECT * FROM users <where><if test=\"name\">AND name = #{name} </if><if test=\"age\">AND age = #{age}</if></where>";
    
    // Test with both conditions
    QVariantMap params1;
    params1["name"] = "John";
    params1["age"] = 25;
    QString result1 = processor->process(sql, params1);
    QString expected1 = "SELECT * FROM users WHERE name = :name AND age = :age";
    QCOMPARE(result1, expected1);
    
    // Test with only name condition (should remove leading AND)
    QVariantMap params2;
    params2["name"] = "John";
    QString result2 = processor->process(sql, params2);
    QString expected2 = "SELECT * FROM users WHERE name = :name";
    QCOMPARE(result2, expected2);
    
    // Test with no conditions (should remove WHERE)
    QVariantMap params3;
    QString result3 = processor->process(sql, params3);
    QString expected3 = "SELECT * FROM users";
    QCOMPARE(result3, expected3);
}

void TestDynamicSqlProcessor::testSetClause()
{
    QString sql = "UPDATE users <set><if test=\"name\">name = #{name}, </if><if test=\"age\">age = #{age},</if></set> WHERE id = #{id}";
    
    QVariantMap params;
    params["name"] = "John";
    params["age"] = 25;
    params["id"] = 1;
    
    QString result = processor->process(sql, params);
    QString expected = "UPDATE users SET name = :name, age = :age WHERE id = :id";
    QCOMPARE(result, expected);
}

QTEST_MAIN(TestDynamicSqlProcessor)
#include "run_dynamicsqlprocessor_test.moc"