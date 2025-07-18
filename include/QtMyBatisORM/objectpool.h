#pragma once

#include <QStack>
#include <QMutex>
#include <QMutexLocker>

namespace QtMyBatisORM {

/**
 * @brief 通用对象池模板类
 * 
 * 用于管理频繁创建和销毁的对象，减少内存分配开销
 * 
 * @tparam T 对象类型
 */
template<typename T>
class ObjectPool
{
public:
    /**
     * @brief 构造函数
     * @param initialSize 初始对象数量
     * @param maxSize 最大对象数量
     */
    ObjectPool(int initialSize = 10, int maxSize = 100)
        : m_maxSize(maxSize)
    {
        // 预创建对象
        for (int i = 0; i < initialSize; ++i) {
            m_availableObjects.push(new T());
        }
        m_totalCreated = initialSize;
    }
    
    /**
     * @brief 析构函数
     */
    ~ObjectPool()
    {
        // 清理所有对象
        QMutexLocker locker(&m_mutex);
        while (!m_availableObjects.isEmpty()) {
            delete m_availableObjects.pop();
        }
    }
    
    /**
     * @brief 获取一个对象
     * @return 对象指针，如果池已满且无可用对象则返回nullptr
     */
    T* acquire()
    {
        QMutexLocker locker(&m_mutex);
        if (m_availableObjects.isEmpty()) {
            // 如果未达到最大数量，则创建新对象
            if (m_totalCreated < m_maxSize) {
                m_totalCreated++;
                return new T();
            }
            // 已达到最大数量，返回nullptr
            return nullptr;
        }
        return m_availableObjects.pop();
    }
    
    /**
     * @brief 归还一个对象到池中
     * @param object 要归还的对象指针
     */
    void release(T* object)
    {
        if (!object) return;
        
        QMutexLocker locker(&m_mutex);
        m_availableObjects.push(object);
    }
    
    /**
     * @brief 获取当前可用对象数量
     * @return 可用对象数量
     */
    int availableCount() const
    {
        QMutexLocker locker(&m_mutex);
        return m_availableObjects.size();
    }
    
    /**
     * @brief 获取已创建的对象总数
     * @return 已创建的对象总数
     */
    int totalCreated() const
    {
        QMutexLocker locker(&m_mutex);
        return m_totalCreated;
    }
    
private:
    QStack<T*> m_availableObjects;
    mutable QMutex m_mutex;
    int m_totalCreated = 0;
    int m_maxSize;
};

} // namespace QtMyBatisORM