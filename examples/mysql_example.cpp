#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QtMyBatisORM/qtmybatisorm.h>
#include <QtMyBatisORM/session.h>
#include <QtMyBatisORM/DataModels.h>

// 定义一个简单的产品类
class Product {
public:
    int id;
    QString name;
    double price;
    int stock;
    
    Product() : id(0), price(0.0), stock(0) {}
    
    Product(int id, const QString& name, double price, int stock)
        : id(id), name(name), price(price), stock(stock) {}
    
    // 转换为QVariantMap，用于插入数据库
    QVariantMap toMap() const {
        QVariantMap map;
        map["id"] = id;
        map["name"] = name;
        map["price"] = price;
        map["stock"] = stock;
        return map;
    }
    
    // 从QVariantMap创建Product对象，用于从数据库读取
    static Product fromMap(const QVariantMap& map) {
        Product product;
        product.id = map["id"].toInt();
        product.name = map["name"].toString();
        product.price = map["price"].toDouble();
        product.stock = map["stock"].toInt();
        return product;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 创建ORM实例，使用MySQL数据库
    // 注意：需要替换为实际的MySQL连接信息
    QSharedPointer<QtMyBatisORM::QtMyBatisORM> orm = QtMyBatisORM::QtMyBatisORM::createMySQL(
        "localhost",     // 主机名
        3306,           // 端口
        "qtmybatis_db", // 数据库名
        "root",         // 用户名
        "password"      // 密码
    );
    
    if (!orm) {
        qCritical() << "Failed to create QtMyBatisORM instance";
        return 1;
    }
    
    qDebug() << "QtMyBatisORM initialized successfully with MySQL database";
    
    // 获取会话
    QSharedPointer<QtMyBatisORM::Session> session = orm->openSession();
    
    if (!session) {
        qCritical() << "Failed to open session";
        return 1;
    }
    
    qDebug() << "Session opened successfully";
    
    // 执行一些基本操作
    try {
        // 开始事务
        session->beginTransaction();
        
        // 创建表
        session->execute("CREATE TABLE IF NOT EXISTS products ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "name VARCHAR(100) NOT NULL, "
                        "price DECIMAL(10,2) NOT NULL, "
                        "stock INT NOT NULL DEFAULT 0)");
        qDebug() << "Table created successfully";
        
        // 清空表
        session->execute("TRUNCATE TABLE products");
        qDebug() << "Table truncated";
        
        // 插入多个产品
        QList<Product> productsToInsert = {
            Product(0, "Laptop", 999.99, 10),
            Product(0, "Smartphone", 499.99, 20),
            Product(0, "Tablet", 299.99, 15),
            Product(0, "Headphones", 99.99, 30)
        };
        
        for (const Product& product : productsToInsert) {
            QVariantMap productMap = product.toMap();
            // 不包含ID，因为它是自动递增的
            productMap.remove("id");
            
            session->execute("INSERT INTO products (name, price, stock) VALUES (:name, :price, :stock)", productMap);
        }
        
        // 提交事务
        session->commit();
        qDebug() << "Products inserted successfully";
        
        // 查询所有产品
        QVariantList productVars = session->selectList("SELECT * FROM products");
        qDebug() << "Found" << productVars.size() << "products:";
        
        QList<Product> products;
        for (const QVariant& productVar : productVars) {
            Product product = Product::fromMap(productVar.toMap());
            products.append(product);
            qDebug() << "Product:" << product.id << product.name << product.price << product.stock;
        }
        
        // 使用动态SQL进行查询
        QVariantMap params;
        params["minPrice"] = 300.0;
        params["maxPrice"] = 1000.0;
        
        QString dynamicSql = "SELECT * FROM products WHERE 1=1";
        if (params.contains("minPrice")) {
            dynamicSql += " AND price >= :minPrice";
        }
        if (params.contains("maxPrice")) {
            dynamicSql += " AND price <= :maxPrice";
        }
        
        QVariantList filteredProducts = session->selectList(dynamicSql, params);
        qDebug() << "Found" << filteredProducts.size() << "products with price between" 
                 << params["minPrice"].toDouble() << "and" << params["maxPrice"].toDouble();
        
        for (const QVariant& productVar : filteredProducts) {
            QVariantMap productMap = productVar.toMap();
            qDebug() << "Filtered product:" << productMap["id"].toInt() 
                     << productMap["name"].toString() << productMap["price"].toDouble();
        }
        
        // 批量更新库存
        session->beginTransaction();
        
        for (Product& product : products) {
            // 增加库存
            product.stock += 5;
            
            QVariantMap updateMap;
            updateMap["id"] = product.id;
            updateMap["stock"] = product.stock;
            
            session->execute("UPDATE products SET stock = :stock WHERE id = :id", updateMap);
        }
        
        session->commit();
        qDebug() << "Stock updated successfully";
        
        // 验证更新
        QVariantList updatedProducts = session->selectList("SELECT * FROM products");
        qDebug() << "Updated products:";
        
        for (const QVariant& productVar : updatedProducts) {
            QVariantMap productMap = productVar.toMap();
            qDebug() << "Product:" << productMap["id"].toInt() 
                     << productMap["name"].toString() << productMap["stock"].toInt();
        }
        
    } catch (const QtMyBatisORM::QtMyBatisException& e) {
        qCritical() << "Error:" << e.message();
        
        // 如果发生错误，回滚事务
        if (session->isInTransaction()) {
            session->rollback();
            qDebug() << "Transaction rolled back";
        }
        
        return 1;
    }
    
    // 关闭会话
    orm->closeSession(session);
    qDebug() << "Session closed successfully";
    
    return 0;
}