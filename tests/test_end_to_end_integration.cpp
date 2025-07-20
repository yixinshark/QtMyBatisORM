#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QRandomGenerator>

#include "QtMyBatisORM/qtmybatisorm.h"
#include "QtMyBatisORM/session.h"
#include "QtMyBatisORM/sessionfactory.h"
#include "QtMyBatisORM/mapperregistry.h"
#include "QtMyBatisORM/cachemanager.h"
#include "QtMyBatisORM/datamodels.h"
#include "QtMyBatisORM/qtmybatisexception.h"

using namespace QtMyBatisORM;

// 测试用的Mapper接口
class ProductMapper : public QObject
{
    Q_OBJECT
    
public:
    explicit ProductMapper(QObject* parent = nullptr) : QObject(parent) {}
    
    virtual QVariantList findAll() { return QVariantList(); }
    virtual QVariant findById(int id) { Q_UNUSED(id); return QVariant(); }
    virtual QVariantList findByPriceRange(double minPrice, double maxPrice) { Q_UNUSED(minPrice); Q_UNUSED(maxPrice); return QVariantList(); }
    virtual int insert(const QVariantMap& product) { Q_UNUSED(product); return 0; }
    virtual int update(const QVariantMap& product) { Q_UNUSED(product); return 0; }
    virtual int deleteById(int id) { Q_UNUSED(id); return 0; }
    virtual int updateStock(int id, int quantity) { Q_UNUSED(id); Q_UNUSED(quantity); return 0; }
};

class OrderMapper : public QObject
{
    Q_OBJECT
    
public:
    explicit OrderMapper(QObject* parent = nullptr) : QObject(parent) {}
    
    virtual QVariantList findAll() { return QVariantList(); }
    virtual QVariant findById(int id) { Q_UNUSED(id); return QVariant(); }
    virtual QVariantList findByCustomerId(int customerId) { Q_UNUSED(customerId); return QVariantList(); }
    virtual int insert(const QVariantMap& order) { Q_UNUSED(order); return 0; }
    virtual int update(const QVariantMap& order) { Q_UNUSED(order); return 0; }
    virtual int deleteById(int id) { Q_UNUSED(id); return 0; }
};

// 并发测试用的工作线程
class WorkerThread : public QThread
{
    Q_OBJECT
    
public:
    WorkerThread(QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm, int workerId, int operationCount)
        : m_orm(orm), m_workerId(workerId), m_operationCount(operationCount), m_success(true) {}
    
    bool isSuccessful() const { return m_success; }
    
protected:
    void run() override;
    
private:
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> m_orm;
    int m_workerId;
    int m_operationCount;
    bool m_success;
};

void WorkerThread::run()
{
    try {
        // 获取会话
        auto session = m_orm->openSession();
        
        for (int i = 0; i < m_operationCount; ++i) {
            // 随机选择操作类型
            int operation = QRandomGenerator::global()->bounded(4);
            
            switch (operation) {
                case 0: { // 查询产品
                    int productId = QRandomGenerator::global()->bounded(10) + 1;
                    QVariantMap params;
                    params["id"] = productId;
                    QVariant product = session->selectOne("ProductMapper.findById", params);
                    if (product.isNull()) {
                        qWarning() << "Worker" << m_workerId << "查询产品" << productId << "失败";
                    }
                    break;
                }
                case 1: { // 查询订单
                    int customerId = QRandomGenerator::global()->bounded(5) + 1;
                    QVariantMap params;
                    params["customerId"] = customerId;
                    QVariantList orders = session->selectList("OrderMapper.findByCustomerId", params);
                    break;
                }
                case 2: { // 更新产品库存
                    int productId = QRandomGenerator::global()->bounded(10) + 1;
                    int quantity = QRandomGenerator::global()->bounded(10) - 5; // -5到4的随机数
                    
                    QVariantMap params;
                    params["id"] = productId;
                    params["quantity"] = quantity;
                    
                    session->update("ProductMapper.updateStock", params);
                    break;
                }
                case 3: { // 创建新订单
                    int customerId = QRandomGenerator::global()->bounded(5) + 1;
                    int productId = QRandomGenerator::global()->bounded(10) + 1;
                    int quantity = QRandomGenerator::global()->bounded(5) + 1;
                    
                    session->beginTransaction();
                    
                    try {
                        // 检查库存
                        QVariantMap productParams;
                        productParams["id"] = productId;
                        QVariant productVar = session->selectOne("ProductMapper.findById", productParams);
                        
                        if (!productVar.isNull()) {
                            QVariantMap product = productVar.toMap();
                            int stock = product["stock"].toInt();
                            
                            if (stock >= quantity) {
                                // 创建订单
                                QVariantMap orderParams;
                                orderParams["customerId"] = customerId;
                                orderParams["productId"] = productId;
                                orderParams["quantity"] = quantity;
                                orderParams["orderDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                                
                                session->insert("OrderMapper.insert", orderParams);
                                
                                // 更新库存
                                QVariantMap updateParams;
                                updateParams["id"] = productId;
                                updateParams["quantity"] = -quantity;
                                
                                session->update("ProductMapper.updateStock", updateParams);
                            }
                        }
                        
                        session->commit();
                    } catch (...) {
                        session->rollback();
                        throw;
                    }
                    break;
                }
            }
        }
        
        m_orm->closeSession(session);
    } catch (const QtMyBatisException& e) {
        qCritical() << "Worker" << m_workerId << "异常:" << e.message();
        m_success = false;
    } catch (...) {
        qCritical() << "Worker" << m_workerId << "未知异常";
        m_success = false;
    }
}

class TestEndToEndIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 基本功能测试
    void testBasicCRUD();
    void testTransactionCommit();
    void testTransactionRollback();
    
    // XML映射测试
    void testXmlMapperLoading();
    void testMapperUsage();
    
    // 缓存集成测试
    void testCacheIntegration();
    
    // 连接池测试
    void testConnectionPoolReuse();
    
    // 并发测试
    void testConcurrentAccess();
    void testConcurrentTransactions();
    
    // 性能测试
    void testPerformanceBenchmark();
    
    // 错误处理测试
    void testErrorHandling();
    void testInvalidConfiguration();

private:
    void setupTestDatabase();
    void createTestTables();
    void insertTestData();
    void setupXmlMappers();
    
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> m_orm;
    QTemporaryDir m_tempDir;
    QString m_dbPath;
    QString m_configPath;
    QStringList m_mapperPaths;
};

void TestEndToEndIntegration::initTestCase()
{
    // 创建临时目录用于测试文件
    QVERIFY(m_tempDir.isValid());
    
    // 设置数据库路径
    m_dbPath = m_tempDir.path() + "/test.db";
    
    // 创建配置文件
    m_configPath = m_tempDir.path() + "/config.json";
    QFile configFile(m_configPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QJsonObject config;
    config["driverName"] = "QSQLITE";
    config["databaseName"] = m_dbPath;
    config["cacheEnabled"] = true;
    config["maxCacheSize"] = 100;
    config["cacheExpireTime"] = 300;
    config["maxConnections"] = 10;
    config["minConnections"] = 2;
    config["maxIdleTime"] = 60;
    
    QJsonDocument doc(config);
    configFile.write(doc.toJson());
    configFile.close();
    
    // 创建XML映射文件
    setupXmlMappers();
    
    // 创建ORM实例
    m_orm = QtMyBatisORM::create(m_configPath, m_mapperPaths);
    QVERIFY(m_orm != nullptr);
    
    // 设置测试数据库
    setupTestDatabase();
}

void TestEndToEndIntegration::cleanupTestCase()
{
    m_orm.reset();
}

void TestEndToEndIntegration::init()
{
    // 每个测试前的准备工作
}

void TestEndToEndIntegration::cleanup()
{
    // 每个测试后的清理工作
}

void TestEndToEndIntegration::setupXmlMappers()
{
    // 创建产品Mapper XML
    QString productMapperPath = m_tempDir.path() + "/product_mapper.xml";
    QFile productMapperFile(productMapperPath);
    QVERIFY(productMapperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QString productMapperXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="ProductMapper">
    <select id="findAll" resultType="Product">
        SELECT * FROM products
    </select>
    
    <select id="findById" parameterType="int" resultType="Product">
        SELECT * FROM products WHERE id = :id
    </select>
    
    <select id="findByPriceRange" parameterType="map" resultType="Product">
        SELECT * FROM products WHERE price >= :minPrice AND price <= :maxPrice
    </select>
    
    <insert id="insert" parameterType="Product">
        INSERT INTO products (name, price, stock) VALUES (:name, :price, :stock)
    </insert>
    
    <update id="update" parameterType="Product">
        UPDATE products SET name = :name, price = :price, stock = :stock WHERE id = :id
    </update>
    
    <update id="updateStock" parameterType="map">
        UPDATE products SET stock = stock + :quantity WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM products WHERE id = :id
    </delete>
</mapper>)";
    
    productMapperFile.write(productMapperXml.toUtf8());
    productMapperFile.close();
    
    // 创建订单Mapper XML
    QString orderMapperPath = m_tempDir.path() + "/order_mapper.xml";
    QFile orderMapperFile(orderMapperPath);
    QVERIFY(orderMapperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QString orderMapperXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="OrderMapper">
    <select id="findAll" resultType="Order">
        SELECT * FROM orders
    </select>
    
    <select id="findById" parameterType="int" resultType="Order">
        SELECT * FROM orders WHERE id = :id
    </select>
    
    <select id="findByCustomerId" parameterType="int" resultType="Order">
        SELECT * FROM orders WHERE customer_id = :customerId
    </select>
    
    <insert id="insert" parameterType="Order">
        INSERT INTO orders (customer_id, product_id, quantity, order_date) 
        VALUES (:customerId, :productId, :quantity, :orderDate)
    </insert>
    
    <update id="update" parameterType="Order">
        UPDATE orders SET customer_id = :customerId, product_id = :productId, 
        quantity = :quantity, order_date = :orderDate WHERE id = :id
    </update>
    
    <delete id="deleteById" parameterType="int">
        DELETE FROM orders WHERE id = :id
    </delete>
</mapper>)";
    
    orderMapperFile.write(orderMapperXml.toUtf8());
    orderMapperFile.close();
    
    m_mapperPaths << productMapperPath << orderMapperPath;
}

void TestEndToEndIntegration::setupTestDatabase()
{
    auto session = m_orm->openSession();
    
    createTestTables();
    insertTestData();
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::createTestTables()
{
    auto session = m_orm->openSession();
    
    // 创建产品表
    session->execute(R"(
        CREATE TABLE products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            price REAL NOT NULL,
            stock INTEGER NOT NULL
        )
    )");
    
    // 创建订单表
    session->execute(R"(
        CREATE TABLE orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            customer_id INTEGER NOT NULL,
            product_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL,
            order_date TEXT NOT NULL,
            FOREIGN KEY (product_id) REFERENCES products (id)
        )
    )");
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::insertTestData()
{
    auto session = m_orm->openSession();
    
    // 插入产品数据
    QVariantList products = {
        QVariantMap{{"name", "Laptop"}, {"price", 999.99}, {"stock", 10}},
        QVariantMap{{"name", "Mouse"}, {"price", 29.99}, {"stock", 20}},
        QVariantMap{{"name", "Keyboard"}, {"price", 79.99}, {"stock", 15}},
        QVariantMap{{"name", "Monitor"}, {"price", 299.99}, {"stock", 5}},
        QVariantMap{{"name", "Headphones"}, {"price", 149.99}, {"stock", 8}},
        QVariantMap{{"name", "Printer"}, {"price", 199.99}, {"stock", 3}},
        QVariantMap{{"name", "Webcam"}, {"price", 59.99}, {"stock", 12}},
        QVariantMap{{"name", "Speakers"}, {"price", 89.99}, {"stock", 7}},
        QVariantMap{{"name", "External SSD"}, {"price", 129.99}, {"stock", 9}},
        QVariantMap{{"name", "Graphics Card"}, {"price", 499.99}, {"stock", 2}}
    };
    
    for (const QVariantMap& product : products) {
        session->insert("ProductMapper.insert", product);
    }
    
    // 插入订单数据
    QVariantList orders = {
        QVariantMap{{"customerId", 1}, {"productId", 1}, {"quantity", 1}, {"orderDate", "2025-07-15T10:30:00"}},
        QVariantMap{{"customerId", 2}, {"productId", 3}, {"quantity", 2}, {"orderDate", "2025-07-16T11:45:00"}},
        QVariantMap{{"customerId", 3}, {"productId", 5}, {"quantity", 1}, {"orderDate", "2025-07-17T09:15:00"}},
        QVariantMap{{"customerId", 1}, {"productId", 7}, {"quantity", 3}, {"orderDate", "2025-07-17T14:20:00"}},
        QVariantMap{{"customerId", 4}, {"productId", 2}, {"quantity", 2}, {"orderDate", "2025-07-18T08:00:00"}}
    };
    
    for (const QVariantMap& order : orders) {
        session->insert("OrderMapper.insert", order);
    }
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testBasicCRUD()
{
    auto session = m_orm->openSession();
    
    // 测试查询
    QVariantList allProducts = session->selectList("ProductMapper.findAll");
    QCOMPARE(allProducts.size(), 10);
    
    // 测试按ID查询
    QVariantMap params;
    params["id"] = 1;
    QVariant product = session->selectOne("ProductMapper.findById", params);
    QVERIFY(!product.isNull());
    QVariantMap productMap = product.toMap();
    QCOMPARE(productMap["name"].toString(), QString("Laptop"));
    
    // 测试插入
    QVariantMap newProduct;
    newProduct["name"] = "USB Hub";
    newProduct["price"] = 39.99;
    newProduct["stock"] = 15;
    
    int insertResult = session->insert("ProductMapper.insert", newProduct);
    QVERIFY(insertResult > 0);
    
    // 验证插入成功
    params["id"] = insertResult;
    product = session->selectOne("ProductMapper.findById", params);
    QVERIFY(!product.isNull());
    productMap = product.toMap();
    QCOMPARE(productMap["name"].toString(), QString("USB Hub"));
    
    // 测试更新
    productMap["price"] = 34.99;
    productMap["stock"] = 20;
    
    int updateResult = session->update("ProductMapper.update", productMap);
    QCOMPARE(updateResult, 1);
    
    // 验证更新成功
    product = session->selectOne("ProductMapper.findById", params);
    QVERIFY(!product.isNull());
    QVariantMap updatedProduct = product.toMap();
    QCOMPARE(updatedProduct["price"].toDouble(), 34.99);
    QCOMPARE(updatedProduct["stock"].toInt(), 20);
    
    // 测试删除
    int deleteResult = session->deleteOne("ProductMapper.deleteById", params);
    QCOMPARE(deleteResult, 1);
    
    // 验证删除成功
    product = session->selectOne("ProductMapper.findById", params);
    QVERIFY(product.isNull());
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testTransactionCommit()
{
    auto session = m_orm->openSession();
    
    // 开始事务
    session->beginTransaction();
    
    try {
        // 添加新产品
        QVariantMap newProduct;
        newProduct["name"] = "Tablet";
        newProduct["price"] = 349.99;
        newProduct["stock"] = 7;
        
        int productId = session->insert("ProductMapper.insert", newProduct);
        QVERIFY(productId > 0);
        
        // 添加新订单
        QVariantMap newOrder;
        newOrder["customerId"] = 5;
        newOrder["productId"] = productId;
        newOrder["quantity"] = 2;
        newOrder["orderDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        int orderId = session->insert("OrderMapper.insert", newOrder);
        QVERIFY(orderId > 0);
        
        // 更新库存
        QVariantMap stockUpdate;
        stockUpdate["id"] = productId;
        stockUpdate["quantity"] = -2;
        
        int updateResult = session->update("ProductMapper.updateStock", stockUpdate);
        QCOMPARE(updateResult, 1);
        
        // 提交事务
        session->commit();
        
        // 验证事务提交成功
        QVariantMap params;
        params["id"] = productId;
        QVariant product = session->selectOne("ProductMapper.findById", params);
        QVERIFY(!product.isNull());
        
        QVariantMap productMap = product.toMap();
        QCOMPARE(productMap["stock"].toInt(), 5); // 7 - 2 = 5
        
        params["id"] = orderId;
        QVariant order = session->selectOne("OrderMapper.findById", params);
        QVERIFY(!order.isNull());
        
    } catch (const QtMyBatisException& e) {
        session->rollback();
        QFAIL(QString("事务测试失败: %1").arg(e.message()).toLocal8Bit());
    }
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testTransactionRollback()
{
    auto session = m_orm->openSession();
    
    // 获取初始产品数量
    QVariantList initialProducts = session->selectList("ProductMapper.findAll");
    int initialCount = initialProducts.size();
    
    // 开始事务
    session->beginTransaction();
    
    // 添加新产品
    QVariantMap newProduct;
    newProduct["name"] = "Smart Watch";
    newProduct["price"] = 249.99;
    newProduct["stock"] = 5;
    
    int productId = session->insert("ProductMapper.insert", newProduct);
    QVERIFY(productId > 0);
    
    // 回滚事务
    session->rollback();
    
    // 验证事务回滚成功
    QVariantList productsAfterRollback = session->selectList("ProductMapper.findAll");
    QCOMPARE(productsAfterRollback.size(), initialCount);
    
    QVariantMap params;
    params["id"] = productId;
    QVariant product = session->selectOne("ProductMapper.findById", params);
    QVERIFY(product.isNull()); // 产品应该不存在
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testXmlMapperLoading()
{
    // 验证XML映射文件已正确加载
    auto session = m_orm->openSession();
    
    // 测试产品映射
    QVariantMap priceRange;
    priceRange["minPrice"] = 100.0;
    priceRange["maxPrice"] = 300.0;
    
    QVariantList products = session->selectList("ProductMapper.findByPriceRange", priceRange);
    QVERIFY(products.size() > 0);
    
    // 验证查询结果
    for (const QVariant& productVar : products) {
        QVariantMap product = productVar.toMap();
        double price = product["price"].toDouble();
        QVERIFY(price >= 100.0 && price <= 300.0);
    }
    
    // 测试订单映射
    QVariantMap customerParams;
    customerParams["customerId"] = 1;
    
    QVariantList orders = session->selectList("OrderMapper.findByCustomerId", customerParams);
    QVERIFY(orders.size() > 0);
    
    // 验证查询结果
    for (const QVariant& orderVar : orders) {
        QVariantMap order = orderVar.toMap();
        QCOMPARE(order["customer_id"].toInt(), 1);
    }
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testMapperUsage()
{
    auto session = m_orm->openSession();
    
    // 获取产品Mapper
    ProductMapper* productMapper = session->getMapper<ProductMapper>();
    QVERIFY(productMapper != nullptr);
    
    // 测试findAll方法
    QVariantList products = productMapper->findAll();
    QVERIFY(products.size() > 0);
    
    // 测试findById方法
    QVariant product = productMapper->findById(1);
    QVERIFY(!product.isNull());
    
    // 测试findByPriceRange方法
    QVariantList priceRangeProducts = productMapper->findByPriceRange(100.0, 300.0);
    QVERIFY(priceRangeProducts.size() > 0);
    
    // 测试insert方法
    QVariantMap newProduct;
    newProduct["name"] = "Wireless Charger";
    newProduct["price"] = 45.99;
    newProduct["stock"] = 25;
    
    int insertResult = productMapper->insert(newProduct);
    QVERIFY(insertResult > 0);
    
    // 测试update方法
    QVariantMap updateProduct;
    updateProduct["id"] = insertResult;
    updateProduct["name"] = "Fast Wireless Charger";
    updateProduct["price"] = 49.99;
    updateProduct["stock"] = 25;
    
    int updateResult = productMapper->update(updateProduct);
    QCOMPARE(updateResult, 1);
    
    // 测试updateStock方法
    int stockUpdateResult = productMapper->updateStock(insertResult, -5);
    QCOMPARE(stockUpdateResult, 1);
    
    // 验证库存更新
    QVariant updatedProduct = productMapper->findById(insertResult);
    QVERIFY(!updatedProduct.isNull());
    QVariantMap updatedProductMap = updatedProduct.toMap();
    QCOMPARE(updatedProductMap["stock"].toInt(), 20); // 25 - 5 = 20
    
    // 测试deleteById方法
    int deleteResult = productMapper->deleteById(insertResult);
    QCOMPARE(deleteResult, 1);
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testCacheIntegration()
{
    auto session = m_orm->openSession();
    
    // 第一次查询 - 应该从数据库获取
    QVariantMap params;
    params["id"] = 1;
    
    QElapsedTimer timer;
    timer.start();
    QVariant product1 = session->selectOne("ProductMapper.findById", params);
    qint64 firstQueryTime = timer.elapsed();
    
    QVERIFY(!product1.isNull());
    
    // 第二次查询 - 应该从缓存获取
    timer.restart();
    QVariant product2 = session->selectOne("ProductMapper.findById", params);
    qint64 secondQueryTime = timer.elapsed();
    
    QVERIFY(!product2.isNull());
    QCOMPARE(product1, product2);
    
    // 缓存查询应该更快
    qDebug() << "First query time:" << firstQueryTime << "ms";
    qDebug() << "Second query time (cached):" << secondQueryTime << "ms";
    
    // 更新产品，应该使缓存失效
    QVariantMap productMap = product1.toMap();
    productMap["name"] = "Updated Laptop";
    
    session->update("ProductMapper.update", productMap);
    
    // 再次查询，应该从数据库获取新数据
    timer.restart();
    QVariant product3 = session->selectOne("ProductMapper.findById", params);
    qint64 thirdQueryTime = timer.elapsed();
    
    QVERIFY(!product3.isNull());
    QVariantMap updatedMap = product3.toMap();
    QCOMPARE(updatedMap["name"].toString(), QString("Updated Laptop"));
    
    qDebug() << "Third query time (after update):" << thirdQueryTime << "ms";
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testConnectionPoolReuse()
{
    // 创建多个会话，验证连接池复用
    QList<QSharedPointer<Session>> sessions;
    
    // 创建多个会话
    for (int i = 0; i < 5; ++i) {
        auto session = m_orm->openSession();
        QVERIFY(session != nullptr);
        
        // 执行简单查询验证会话可用
        QVariantList products = session->selectList("ProductMapper.findAll");
        QVERIFY(products.size() > 0);
        
        sessions.append(session);
    }
    
    // 关闭会话
    for (auto& session : sessions) {
        m_orm->closeSession(session);
    }
    
    // 再次创建会话，应该复用连接
    auto newSession = m_orm->openSession();
    QVERIFY(newSession != nullptr);
    
    QVariantList products = newSession->selectList("ProductMapper.findAll");
    QVERIFY(products.size() > 0);
    
    m_orm->closeSession(newSession);
}

void TestEndToEndIntegration::testConcurrentAccess()
{
    // 创建多个工作线程并发访问数据库
    const int threadCount = 10;
    const int operationsPerThread = 50;
    
    QList<WorkerThread*> threads;
    
    for (int i = 0; i < threadCount; ++i) {
        WorkerThread* thread = new WorkerThread(m_orm, i, operationsPerThread);
        threads.append(thread);
        thread->start();
    }
    
    // 等待所有线程完成
    for (WorkerThread* thread : threads) {
        thread->wait();
        QVERIFY(thread->isSuccessful());
        delete thread;
    }
    
    // 验证数据库一致性
    auto session = m_orm->openSession();
    
    // 检查产品库存是否合理
    QVariantList products = session->selectList("ProductMapper.findAll");
    for (const QVariant& productVar : products) {
        QVariantMap product = productVar.toMap();
        int stock = product["stock"].toInt();
        QVERIFY(stock >= 0); // 库存不应为负
    }
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testConcurrentTransactions()
{
    // 创建多个线程同时执行事务
    const int threadCount = 5;
    QList<QThread*> threads;
    QList<bool> results(threadCount, false);
    
    for (int i = 0; i < threadCount; ++i) {
        QThread* thread = QThread::create([this, i, &results]() {
            try {
                auto session = m_orm->openSession();
                
                // 开始事务
                session->beginTransaction();
                
                try {
                    // 获取产品
                    QVariantMap params;
                    params["id"] = (i % 5) + 1; // 使用不同的产品ID
                    
                    QVariant productVar = session->selectOne("ProductMapper.findById", params);
                    if (productVar.isNull()) {
                        session->rollback();
                        return;
                    }
                    
                    QVariantMap product = productVar.toMap();
                    int currentStock = product["stock"].toInt();
                    
                    // 更新库存
                    QVariantMap updateParams;
                    updateParams["id"] = params["id"];
                    updateParams["quantity"] = -1; // 减少1个库存
                    
                    if (currentStock > 0) {
                        session->update("ProductMapper.updateStock", updateParams);
                        
                        // 创建订单
                        QVariantMap orderParams;
                        orderParams["customerId"] = (i % 3) + 1;
                        orderParams["productId"] = params["id"];
                        orderParams["quantity"] = 1;
                        orderParams["orderDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                        
                        session->insert("OrderMapper.insert", orderParams);
                        
                        // 提交事务
                        session->commit();
                        results[i] = true;
                    } else {
                        // 库存不足，回滚
                        session->rollback();
                    }
                } catch (...) {
                    session->rollback();
                    throw;
                }
                
                m_orm->closeSession(session);
            } catch (const QtMyBatisException& e) {
                qCritical() << "Thread" << i << "异常:" << e.message();
            }
        });
        
        threads.append(thread);
        thread->start();
    }
    
    // 等待所有线程完成
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    
    // 验证至少有一些事务成功
    bool anySuccess = false;
    for (bool result : results) {
        if (result) {
            anySuccess = true;
            break;
        }
    }
    
    QVERIFY(anySuccess);
    
    // 验证数据库一致性
    auto session = m_orm->openSession();
    
    // 检查产品库存是否合理
    QVariantList products = session->selectList("ProductMapper.findAll");
    for (const QVariant& productVar : products) {
        QVariantMap product = productVar.toMap();
        int stock = product["stock"].toInt();
        QVERIFY(stock >= 0); // 库存不应为负
    }
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testPerformanceBenchmark()
{
    auto session = m_orm->openSession();
    
    // 测试批量插入性能
    QElapsedTimer timer;
    timer.start();
    
    const int batchSize = 100;
    session->beginTransaction();
    
    try {
        for (int i = 0; i < batchSize; ++i) {
            QVariantMap product;
            product["name"] = QString("Benchmark Product %1").arg(i);
            product["price"] = 10.0 + (i % 90);
            product["stock"] = 50;
            
            session->insert("ProductMapper.insert", product);
        }
        
        session->commit();
    } catch (...) {
        session->rollback();
        throw;
    }
    
    qint64 insertTime = timer.elapsed();
    qDebug() << "Batch insert time for" << batchSize << "records:" << insertTime << "ms";
    qDebug() << "Average insert time per record:" << (insertTime / batchSize) << "ms";
    
    // 测试批量查询性能
    timer.restart();
    
    for (int i = 0; i < 10; ++i) {
        QVariantList products = session->selectList("ProductMapper.findAll");
    }
    
    qint64 queryTime = timer.elapsed();
    qDebug() << "Time for 10 full table queries:" << queryTime << "ms";
    qDebug() << "Average query time:" << (queryTime / 10) << "ms";
    
    // 测试缓存性能
    timer.restart();
    
    QVariantMap params;
    params["id"] = 1;
    
    // 第一次查询（未缓存）
    session->selectOne("ProductMapper.findById", params);
    qint64 uncachedTime = timer.elapsed();
    
    // 第二次查询（已缓存）
    timer.restart();
    session->selectOne("ProductMapper.findById", params);
    qint64 cachedTime = timer.elapsed();
    
    qDebug() << "Uncached query time:" << uncachedTime << "ms";
    qDebug() << "Cached query time:" << cachedTime << "ms";
    qDebug() << "Cache speedup factor:" << (uncachedTime > 0 ? (double)uncachedTime / cachedTime : 0);
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testErrorHandling()
{
    auto session = m_orm->openSession();
    
    // 测试无效SQL
    QVERIFY_EXCEPTION_THROWN(
        session->execute("INVALID SQL STATEMENT"),
        SqlExecutionException
    );
    
    // 测试无效参数
    QVariantMap params;
    params["invalid_param"] = "value";
    
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("ProductMapper.findById", params),
        SqlExecutionException
    );
    
    // 测试无效映射ID
    QVERIFY_EXCEPTION_THROWN(
        session->selectOne("ProductMapper.nonExistentMethod", QVariantMap()),
        MappingException
    );
    
    // 测试在关闭的会话上执行操作
    session->close();
    
    QVERIFY_EXCEPTION_THROWN(
        session->selectList("ProductMapper.findAll"),
        SqlExecutionException
    );
    
    m_orm->closeSession(session);
}

void TestEndToEndIntegration::testInvalidConfiguration()
{
    // 测试无效的数据库配置
    QString invalidConfigPath = m_tempDir.path() + "/invalid_config.json";
    QFile configFile(invalidConfigPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QJsonObject config;
    config["driverName"] = "INVALID_DRIVER";
    config["databaseName"] = "/nonexistent/path/db.sqlite";
    
    QJsonDocument doc(config);
    configFile.write(doc.toJson());
    configFile.close();
    
    // 尝试创建ORM实例，应该抛出异常
    QVERIFY_EXCEPTION_THROWN(
        QtMyBatisORM::create(invalidConfigPath, QStringList()),
        ConfigurationException
    );
}

