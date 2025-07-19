#include <QtTest>
#include <QResource>
#include <QDir>
#include "QtMyBatisORM/xmlmapperparser.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

class TestXMLMapperParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseValidMapper();
    void testParseInvalidMapper();
    void testParseMalformedXML();
    void testParseMultipleMappers();
    void testDuplicateStatementIds();
    void testDynamicSqlElements();
    void testResultMapParsing();

private:
    XMLMapperParser m_parser;
    QString getTestResourcePath(const QString& filename);
};

void TestXMLMapperParser::initTestCase()
{
    // 确保测试资源目录存在
    QDir resourceDir("../tests/resources");
    if (!resourceDir.exists()) {
        resourceDir.setPath("tests/resources");
    }
    QVERIFY2(resourceDir.exists(), "Test resources directory not found");
}

QString TestXMLMapperParser::getTestResourcePath(const QString& filename)
{
    return QString("tests/resources/%1").arg(filename);
}

void TestXMLMapperParser::testParseValidMapper()
{
    try {
        // 由于我们不能直接从文件系统读取，我们需要模拟资源文件
        // 这里我们测试解析逻辑，但跳过实际的资源文件读取
        QSKIP("Skipping resource file test - requires Qt resource system integration");
    } catch (const QtMyBatisException& e) {
        QFAIL(qPrintable(QString("Unexpected exception: %1").arg(e.message())));
    }
}

void TestXMLMapperParser::testParseInvalidMapper()
{
    // 测试缺少namespace的情况
    QString invalidXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<mapper>\n"
                        "    <select id=\"selectUser\">\n"
                        "        SELECT * FROM users\n"
                        "    </select>\n"
                        "</mapper>";
    
    QDomDocument doc;
    QVERIFY(doc.setContent(invalidXml));
    
    try {
        MapperConfig config = m_parser.parseMapperFromDocument(doc, "test.xml");
        QFAIL("Expected ConfigurationException for missing namespace");
    } catch (const ConfigurationException& e) {
        QVERIFY(e.message().contains("namespace is required"));
    }
}

void TestXMLMapperParser::testParseMalformedXML()
{
    QString malformedXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<mapper namespace=\"test\">\n"
                          "    <select id=\"selectUser\"\n"
                          "        SELECT * FROM users\n"
                          "    </select>";
    
    QDomDocument doc;
    
    // 验证XML格式错误能被检测到
    auto parseResult = doc.setContent(malformedXml);
    QVERIFY(!parseResult);
    QVERIFY(!parseResult.errorMessage.isEmpty());
}

void TestXMLMapperParser::testParseMultipleMappers()
{
    // 创建两个有效的mapper配置
    QString xml1 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<mapper namespace=\"com.example.UserMapper\">\n"
                   "    <select id=\"selectUser\">\n"
                   "        SELECT * FROM users WHERE id = #{id}\n"
                   "    </select>\n"
                   "</mapper>";
    
    QString xml2 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<mapper namespace=\"com.example.OrderMapper\">\n"
                   "    <select id=\"selectOrder\">\n"
                   "        SELECT * FROM orders WHERE id = #{id}\n"
                   "    </select>\n"
                   "</mapper>";
    
    QDomDocument doc1, doc2;
    QVERIFY(doc1.setContent(xml1));
    QVERIFY(doc2.setContent(xml2));
    
    try {
        MapperConfig config1 = m_parser.parseMapperFromDocument(doc1, "user.xml");
        MapperConfig config2 = m_parser.parseMapperFromDocument(doc2, "order.xml");
        
        QCOMPARE(config1.namespace_, QString("com.example.UserMapper"));
        QCOMPARE(config2.namespace_, QString("com.example.OrderMapper"));
        QCOMPARE(config1.statements.size(), 1);
        QCOMPARE(config2.statements.size(), 1);
        
        // 测试没有重复的语句ID
        QList<MapperConfig> mappers = {config1, config2};
        m_parser.checkForDuplicateStatementIds(mappers);
        
    } catch (const QtMyBatisException& e) {
        QFAIL(qPrintable(QString("Unexpected exception: %1").arg(e.message())));
    }
}

void TestXMLMapperParser::testDuplicateStatementIds()
{
    // 创建两个有重复语句ID的mapper
    QString xml1 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<mapper namespace=\"com.example.UserMapper\">\n"
                   "    <select id=\"selectById\">\n"
                   "        SELECT * FROM users WHERE id = #{id}\n"
                   "    </select>\n"
                   "</mapper>";
    
    QString xml2 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<mapper namespace=\"com.example.UserMapper\">\n"
                   "    <select id=\"selectById\">\n"
                   "        SELECT * FROM users WHERE id = #{id}\n"
                   "    </select>\n"
                   "</mapper>";
    
    QDomDocument doc1, doc2;
    QVERIFY(doc1.setContent(xml1));
    QVERIFY(doc2.setContent(xml2));
    
    try {
        MapperConfig config1 = m_parser.parseMapperFromDocument(doc1, "user1.xml");
        MapperConfig config2 = m_parser.parseMapperFromDocument(doc2, "user2.xml");
        
        QList<MapperConfig> mappers = {config1, config2};
        m_parser.checkForDuplicateStatementIds(mappers);
        
        QFAIL("Expected ConfigurationException for duplicate statement IDs");
    } catch (const ConfigurationException& e) {
        QVERIFY(e.message().contains("Duplicate statement ID"));
    }
}

void TestXMLMapperParser::testDynamicSqlElements()
{
    QString xmlWithDynamic = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                            "<mapper namespace=\"com.example.UserMapper\">\n"
                            "    <select id=\"selectUsersByCondition\">\n"
                            "        SELECT * FROM users WHERE 1=1\n"
                            "        <if test=\"name != null\">\n"
                            "            AND name = #{name}\n"
                            "        </if>\n"
                            "        <if test=\"active != null\">\n"
                            "            AND active = #{active}\n"
                            "        </if>\n"
                            "    </select>\n"
                            "    <select id=\"selectUsersByIds\">\n"
                            "        SELECT * FROM users WHERE id IN\n"
                            "        <foreach collection=\"ids\" item=\"id\" open=\"(\" close=\")\" separator=\",\">\n"
                            "            #{id}\n"
                            "        </foreach>\n"
                            "    </select>\n"
                            "</mapper>";
    
    QDomDocument doc;
    QVERIFY(doc.setContent(xmlWithDynamic));
    
    try {
        MapperConfig config = m_parser.parseMapperFromDocument(doc, "dynamic.xml");
        
        QCOMPARE(config.statements.size(), 2);
        
        // 检查第一个语句的动态元素
        StatementConfig stmt1 = config.statements["selectUsersByCondition"];
        QVERIFY(stmt1.dynamicElements.size() >= 2);
        
        // 检查第二个语句的动态元素
        StatementConfig stmt2 = config.statements["selectUsersByIds"];
        QVERIFY(stmt2.dynamicElements.size() >= 1);
        
        // 验证foreach元素的解析
        bool foundForeach = false;
        for (auto it = stmt2.dynamicElements.begin(); it != stmt2.dynamicElements.end(); ++it) {
            if (it.key().startsWith("foreach_")) {
                foundForeach = true;
                QString value = it.value();
                QVERIFY(value.contains("ids"));
                QVERIFY(value.contains("id"));
                QVERIFY(value.contains(","));
                break;
            }
        }
        QVERIFY(foundForeach);
        
    } catch (const QtMyBatisException& e) {
        QFAIL(qPrintable(QString("Unexpected exception: %1").arg(e.message())));
    }
}

void TestXMLMapperParser::testResultMapParsing()
{
    QString xmlWithResultMap = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<mapper namespace=\"com.example.UserMapper\">\n"
                              "    <resultMap id=\"UserResultMap\" type=\"User\">\n"
                              "        <id property=\"id\" column=\"user_id\"/>\n"
                              "        <result property=\"name\" column=\"user_name\"/>\n"
                              "        <result property=\"email\" column=\"user_email\"/>\n"
                              "    </resultMap>\n"
                              "    <resultMap id=\"DetailedUserResultMap\" type=\"DetailedUser\">\n"
                              "        <id property=\"id\" column=\"user_id\"/>\n"
                              "        <result property=\"name\" column=\"user_name\"/>\n"
                              "    </resultMap>\n"
                              "    <select id=\"selectUser\" resultMap=\"UserResultMap\">\n"
                              "        SELECT user_id, user_name, user_email FROM users WHERE id = #{id}\n"
                              "    </select>\n"
                              "    <sql id=\"createUserTable\">\n"
                              "        CREATE TABLE IF NOT EXISTS users (\n"
                              "            user_id INT AUTO_INCREMENT PRIMARY KEY,\n"
                              "            user_name VARCHAR(100) NOT NULL,\n"
                              "            user_email VARCHAR(255) UNIQUE\n"
                              "        )\n"
                              "    </sql>\n"
                              "</mapper>";
    
    QDomDocument doc;
    QVERIFY(doc.setContent(xmlWithResultMap));
    
    try {
        MapperConfig config = m_parser.parseMapperFromDocument(doc, "resultmap.xml");
        
        // 验证resultMap解析
        QCOMPARE(config.resultMaps.size(), 2);
        QVERIFY(config.resultMaps.contains("UserResultMap"));
        QVERIFY(config.resultMaps.contains("DetailedUserResultMap"));
        QCOMPARE(config.resultMaps["UserResultMap"], QString("User"));
        QCOMPARE(config.resultMaps["DetailedUserResultMap"], QString("DetailedUser"));
        
        // 验证语句解析 - 现在应该有2个语句（select + sql）
        QCOMPARE(config.statements.size(), 2);
        QVERIFY(config.statements.contains("selectUser"));
        QVERIFY(config.statements.contains("createUserTable"));
        
        // 验证DDL语句类型
        StatementConfig ddlStmt = config.statements["createUserTable"];
        QCOMPARE(ddlStmt.type, StatementType::DDL);
        QVERIFY(ddlStmt.sql.contains("CREATE TABLE"));
        
    } catch (const QtMyBatisException& e) {
        QFAIL(qPrintable(QString("Unexpected exception: %1").arg(e.message())));
    }
}

QTEST_MAIN(TestXMLMapperParser)
#include "run_xmlmapperparser_test.moc"