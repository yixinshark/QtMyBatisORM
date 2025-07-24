#pragma once

#include <QStack>
#include <QMutex>
#include <QMutexLocker>

namespace QtMyBatisORM {

/**
 * @brief Generic object pool template class
 * 
 * Used to manage frequently created and destroyed objects, reducing memory allocation overhead
 * 通用对象池模板类;用于管理频繁创建和销毁的对象，减少内存分配开销
 * @tparam T Object type
 */
template<typename T>
class ObjectPool
{
public:
    /**
     * @brief Constructor
     * @param initialSize Initial number of objects
     * @param maxSize Maximum number of objects
     */
    ObjectPool(int initialSize = 10, int maxSize = 100)
        : m_maxSize(maxSize)
    {
        // Pre-create objects
        for (int i = 0; i < initialSize; ++i) {
            m_availableObjects.push(new T());
        }
        m_totalCreated = initialSize;
    }
    
    ~ObjectPool()
    {
        // Clean up all objects
        QMutexLocker locker(&m_mutex);
        while (!m_availableObjects.isEmpty()) {
            delete m_availableObjects.pop();
        }
    }
    
    /**
     * @brief Acquire an object
     * @return Object pointer, returns nullptr if pool is full and no available objects
     */
    T* acquire()
    {
        QMutexLocker locker(&m_mutex);
        if (m_availableObjects.isEmpty()) {
            // If maximum count not reached, create new object
            if (m_totalCreated < m_maxSize) {
                m_totalCreated++;
                return new T();
            }
            // Maximum count reached, return nullptr
            return nullptr;
        }
        return m_availableObjects.pop();
    }
    
    /**
     * @brief Return an object to the pool
     * @param object Object pointer to return
     */
    void release(T* object)
    {
        if (!object) return;
        
        QMutexLocker locker(&m_mutex);
        m_availableObjects.push(object);
    }
    
    /**
     * @brief Get current available object count
     * @return Available object count
     */
    int availableCount() const
    {
        QMutexLocker locker(&m_mutex);
        return m_availableObjects.size();
    }
    
    /**
     * @brief Get total number of created objects
     * @return Total number of created objects
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