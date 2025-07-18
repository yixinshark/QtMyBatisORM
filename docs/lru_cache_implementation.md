# LRU Cache Implementation

## Overview

This document describes the LRU (Least Recently Used) caching strategy implementation for the QtMyBatisORM CacheManager.

## Features Implemented

### 1. LRU Eviction Strategy
- **Deterministic Ordering**: Uses sequence numbers to ensure consistent eviction order when timestamps are identical
- **Access-Based LRU**: Tracks both creation time (`timestamp`) and last access time (`lastAccessTime`)
- **Capacity Control**: Automatically evicts least recently used items when cache reaches maximum capacity

### 2. Enhanced Cache Statistics
- **Hit Rate Tracking**: Calculates and maintains cache hit rate statistics
- **Eviction Monitoring**: Tracks number of items evicted due to capacity limits
- **Expiration Tracking**: Monitors expired items that are automatically cleaned up
- **Access Statistics**: Records total requests, hits, and misses

### 3. Cache Entry Enhancements
- **Sequence Numbers**: Each cache entry gets a unique sequence number for deterministic LRU ordering
- **Access Tracking**: Records access count and hit count for each entry
- **Time Tracking**: Maintains both creation timestamp and last access time

## Implementation Details

### Data Structures

#### CacheEntry Structure
```cpp
struct CacheEntry
{
    QVariant value;
    QDateTime timestamp;           // Creation time
    QDateTime lastAccessTime;      // Last access time for LRU
    int accessCount = 0;          // Total access count
    int hitCount = 0;             // Hit count for statistics
    qint64 sequenceNumber = 0;    // Sequence for deterministic ordering
};
```

#### CacheStats Structure
```cpp
struct CacheStats
{
    int totalRequests = 0;        // Total cache requests
    int hitCount = 0;             // Cache hits
    int missCount = 0;            // Cache misses
    int evictionCount = 0;        // LRU evictions
    int expiredCount = 0;         // Expired entries cleaned
    double hitRate = 0.0;         // Hit rate percentage
    int currentSize = 0;          // Current cache size
    int maxSize = 0;              // Maximum cache size
    QDateTime lastAccess;         // Last access time
    QDateTime lastEviction;       // Last eviction time
    QDateTime lastExpiration;     // Last expiration cleanup time
};
```

### LRU Algorithm

The LRU eviction algorithm works as follows:

1. **Capacity Check**: When adding a new item, check if cache is at maximum capacity
2. **LRU Selection**: Find the least recently used item by comparing:
   - `lastAccessTime` (primary criterion)
   - `sequenceNumber` (secondary criterion for deterministic ordering)
3. **Eviction**: Remove the selected LRU item and update statistics
4. **Insertion**: Add the new item with updated sequence number

### Key Methods

#### Core Cache Operations
- `put(key, value)`: Adds/updates cache entry with LRU tracking
- `get(key)`: Retrieves value and updates access time for LRU
- `remove(key)`: Removes specific cache entry
- `clear()`: Clears all cache entries

#### LRU-Specific Methods
- `evictLeastRecentlyUsed()`: Implements LRU eviction algorithm
- `getStats()`: Returns comprehensive cache statistics
- `resetStats()`: Resets all statistics counters
- `getHitRate()`: Returns current cache hit rate
- `printStats()`: Prints detailed cache statistics

## Testing

### Test Coverage

The implementation includes comprehensive tests covering:

1. **Basic LRU Eviction**: Verifies that oldest items are evicted when capacity is reached
2. **Access Order Impact**: Tests that accessing items affects LRU ordering
3. **Capacity Control**: Ensures cache never exceeds maximum size
4. **Mixed Operations**: Tests LRU behavior with complex access patterns
5. **Statistics Accuracy**: Verifies all statistics are correctly calculated
6. **Hit Rate Calculation**: Tests hit rate computation accuracy
7. **Eviction Statistics**: Monitors eviction counts and timing
8. **Statistics Reset**: Verifies statistics can be reset without affecting cache content

### Test Results

All tests pass successfully:
- ✅ Basic cache operations (put, get, remove, clear)
- ✅ LRU eviction strategy
- ✅ Capacity control
- ✅ Statistics tracking
- ✅ Hit rate calculation
- ✅ Pattern-based invalidation

## Usage Example

```cpp
// Configure cache with LRU settings
DatabaseConfig config;
config.cacheEnabled = true;
config.maxCacheSize = 100;    // Maximum 100 entries
config.cacheExpireTime = 600; // 10 minutes expiration

// Create cache manager
CacheManager cache(config);

// Add items to cache
cache.put("user:1", userData1);
cache.put("user:2", userData2);

// Access items (affects LRU ordering)
auto user1 = cache.get("user:1");  // user:1 becomes most recently used

// Get statistics
CacheStats stats = cache.getStats();
qDebug() << "Hit Rate:" << (stats.hitRate * 100) << "%";
qDebug() << "Evictions:" << stats.evictionCount;

// Print detailed statistics
cache.printStats();
```

## Performance Characteristics

- **Time Complexity**: O(n) for LRU eviction (where n is cache size)
- **Space Complexity**: O(1) additional space per cache entry
- **Thread Safety**: All operations are thread-safe using QMutex
- **Memory Efficiency**: Minimal overhead for LRU tracking

## Configuration

The LRU cache behavior can be configured through `DatabaseConfig`:

- `cacheEnabled`: Enable/disable caching
- `maxCacheSize`: Maximum number of cache entries
- `cacheExpireTime`: Time-based expiration (seconds, 0 = no expiration)

## Future Enhancements

Potential improvements for the LRU implementation:

1. **LRU-K Algorithm**: Track K most recent accesses instead of just the last one
2. **Adaptive Sizing**: Dynamic cache size adjustment based on hit rates
3. **Segmented LRU**: Separate hot and cold data segments
4. **Compressed Statistics**: More memory-efficient statistics storage
5. **Async Eviction**: Background eviction to reduce operation latency