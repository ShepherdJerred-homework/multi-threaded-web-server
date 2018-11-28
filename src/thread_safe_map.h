
#ifndef MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H
#define MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <vector>
#include <sstream>

using std::unordered_map;
using std::mutex;
using std::lock_guard;
using std::shared_lock;
using std::shared_mutex;
using std::iterator;
using std::function;
using std::string;
using std::for_each;
using std::vector;
using std::stringstream;

template<typename K, typename V>
class thread_safe_map {
private:
    shared_mutex mutex;
    unordered_map<K, V> map;
public:
    V operator[](const K& key) const {
        shared_lock lock(mutex);
        return map[key];
    }

    V& operator[](const K& key) {
        lock_guard lock(mutex);
        return map[key];
    }

    bool contains(const K& key) {
        shared_lock lock(mutex);
        return map.find(key) != map.end();
    }

    void erase(const K& key) {
        lock_guard lock(mutex);
        map.erase(key);
    }

    string toJson() {
        shared_lock lock(mutex);
        stringstream stream;

        bool first = true;
        stream << "[";

        for (auto it = map.begin(); it != map.end(); it++) {
            if (first) {
                first = false;
            } else {
                stream << ", ";
            }
            std::chrono::time_point time_point = it->second;
            stream << "\n  { \"word\" : \"" << it->first << "\",  "
                   << "\"time\" : " << std::chrono::system_clock::to_time_t(time_point) << " }";
        }

        stream << "\n]";
        return stream.str();
    }

    template<typename Function>
    vector<K> find_matching_keys(const Function& f) {
        shared_lock lock(mutex);
        vector<K> matching_keys;
        for (auto it = map.begin(); it != map.end(); it++) {
            if (f(it->second)) {
                matching_keys.push_back(it->first);
            }
        }
        return matching_keys;
    }
};


#endif //MULTI_THREADED_WEB_SERVER_THREAD_SAFE_MAP_H
