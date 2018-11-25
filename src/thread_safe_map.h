
#ifndef MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H
#define MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <iterator>

using std::unordered_map;
using std::mutex;
using std::lock_guard;
using std::shared_lock;
using std::shared_mutex;
using std::iterator;

template<typename K, typename V>
class thread_safe_map {
private:
    shared_mutex mapMutex;
    unordered_map<K, V> map;
public:
    V operator[](K key) const {
        shared_lock lock(mapMutex);
        return map[key];
    }

    V& operator[](K key) {
        lock_guard lock(mapMutex);
        return map[key];
    }

    bool contains(K key) {
        shared_lock lock(mapMutex);
        return map.find(key) != map.end();
    }

    void erase(const K& key) {
        lock_guard lock(mapMutex);
        map.erase(key);
    }

    auto begin() {
        return map.begin();
    }

    auto end()  {
        return map.end();
    }
};


#endif //MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H
