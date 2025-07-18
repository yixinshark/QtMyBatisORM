#include "QtMyBatisORM/connectionpool.h"
#include "QtMyBatisORM/logger.h"
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>
#include <QElapsedTimer>

namespace QtMyBatisORM {

/**
 * @brief 连接池监控器
 * 
 * 监控连接池使用情况并根据使用模式预测需求，动态调整连接池大小
 */
class ConnectionPoolMonitor : public QObject
{
    Q_OBJECT
    
public:
    explicit ConnectionPoolMonitor(ConnectionPool* pool, QObject* parent = nullptr)
        : QObject(parent)
        , m_pool(pool)
        , m_monitoringEnabled(false)
        , m_usageHistory(24, 0) // 保存24个采样点
        , m_currentSampleIndex(0)
    {
        m_monitorTimer = new QTimer(this);
        connect(m_monitorTimer, &QTimer::timeout, this, &ConnectionPoolMonitor::collectUsageStatistics);
    }
    
    void startMonitoring(int sampleIntervalMs = 5000)
    {
        if (!m_monitoringEnabled) {
            m_monitoringEnabled = true;
            m_monitorTimer->start(sampleIntervalMs);
            Logger::info(QStringLiteral("Connection pool monitoring started"), {
                {"sampleIntervalMs", sampleIntervalMs}
            });
        }
    }
    
    void stopMonitoring()
    {
        if (m_monitoringEnabled) {
            m_monitoringEnabled = false;
            m_monitorTimer->stop();
            Logger::info(QStringLiteral("Connection pool monitoring stopped"));
        }
    }
    
    bool isMonitoringEnabled() const
    {
        return m_monitoringEnabled;
    }
    
    QList<int> getUsageHistory() const
    {
        QMutexLocker locker(&m_mutex);
        return m_usageHistory;
    }
    
    double getAverageUsage() const
    {
        QMutexLocker locker(&m_mutex);
        int sum = 0;
        int count = 0;
        
        for (int usage : m_usageHistory) {
            if (usage > 0) {
                sum += usage;
                count++;
            }
        }
        
        return count > 0 ? static_cast<double>(sum) / count : 0.0;
    }
    
    int getPredictedUsage() const
    {
        QMutexLocker locker(&m_mutex);
        
        // 简单的线性预测：基于最近3个采样点的趋势
        if (m_usageHistory.size() < 3) {
            return m_pool->usedConnections();
        }
        
        // 找到最近3个有效的采样点
        QList<int> recentSamples;
        int currentIndex = m_currentSampleIndex;
        
        for (int i = 0; i < m_usageHistory.size() && recentSamples.size() < 3; ++i) {
            int index = (currentIndex - i + m_usageHistory.size()) % m_usageHistory.size();
            if (m_usageHistory[index] > 0) {
                recentSamples.append(m_usageHistory[index]);
            }
        }
        
        if (recentSamples.size() < 3) {
            return m_pool->usedConnections();
        }
        
        // 计算简单的线性趋势
        int trend = (recentSamples[0] - recentSamples[2]) / 2;
        int prediction = recentSamples[0] + trend;
        
        // 确保预测值在合理范围内
        return qMax(1, qMin(prediction, m_pool->totalConnections() * 2));
    }
    
private slots:
    void collectUsageStatistics()
    {
        if (!m_pool) {
            return;
        }
        
        QMutexLocker locker(&m_mutex);
        
        // 获取当前使用情况
        int usedConnections = m_pool->usedConnections();
        int totalConnections = m_pool->totalConnections();
        
        // 更新使用历史
        m_usageHistory[m_currentSampleIndex] = usedConnections;
        m_currentSampleIndex = (m_currentSampleIndex + 1) % m_usageHistory.size();
        
        // 计算使用率
        double usageRatio = totalConnections > 0 ? 
            static_cast<double>(usedConnections) / totalConnections : 0.0;
        
        // 记录使用情况
        Logger::debug(QStringLiteral("Connection pool usage statistics"), {
            {"usedConnections", usedConnections},
            {"totalConnections", totalConnections},
            {"usageRatio", usageRatio}
        });
        
        // 根据使用情况调整连接池
        adjustConnectionPool(usedConnections, totalConnections, usageRatio);
    }
    
    void adjustConnectionPool(int usedConnections, int totalConnections, double usageRatio)
    {
        // 如果使用率高，预创建连接
        if (usageRatio > 0.7) {
            int predictedUsage = getPredictedUsage();
            if (predictedUsage > totalConnections) {
                Logger::info(QStringLiteral("Proactively creating connections based on usage trend"), {
                    {"currentUsage", usedConnections},
                    {"totalConnections", totalConnections},
                    {"predictedUsage", predictedUsage},
                    {"usageRatio", usageRatio}
                });
                
                // 通知连接池预创建连接
                // 注意：这里假设ConnectionPool有一个monitorConnectionUsage方法
                // 实际实现中需要添加这个方法
                m_pool->monitorConnectionUsage();
            }
        }
    }
    
private:
    ConnectionPool* m_pool;
    QTimer* m_monitorTimer;
    bool m_monitoringEnabled;
    QList<int> m_usageHistory;
    int m_currentSampleIndex;
    mutable QMutex m_mutex;
};

} // namespace QtMyBatisORM

#include "connectionpool_monitor.moc"