
#ifndef MULTI_THREADED_WEB_SERVER_CACHE_H
#define MULTI_THREADED_WEB_SERVER_CACHE_H

#include <string>
#include "thread_safe_map.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>

using std::chrono::system_clock;
using std::chrono::seconds;
using std::string;
using std::pair;
using std::cout;
using std::endl;
using std::stringstream;
using std::lock_guard;
using std::shared_lock;
using std::shared_mutex;
using std::vector;

class cache {
private:
    thread_safe_map<string, string> request_cache;
    thread_safe_map<string, system_clock::time_point> request_time;
    shared_mutex m;
    constexpr static auto EXPIRE_TIME = seconds(60);

    static bool isExpired(system_clock::time_point time) {
        return system_clock::now() > time + EXPIRE_TIME;
    }

public:
    void clean() {
        lock_guard lock(m);
        vector<string> expired_keys = request_time.find_matching_keys(isExpired);
        for (auto &key : expired_keys) {
            cout << "Removing " << key << " from cache" << endl;
            request_cache.erase(key);
            request_time.erase(key);
        }
    }

    string toJson() {
        shared_lock lock(m);
        return request_time.toJson();
    }

    bool contains(const string& query) {
        shared_lock lock(m);
        return request_cache.contains(query);
    }

    string get(const string& query) {
        shared_lock lock(m);
        return request_cache[query];
    }

    void set(const string& query, const string& result) {
        lock_guard lock(m);
        request_cache[query] = result;
        request_time[query] = system_clock::now();
    }

    system_clock::time_point getTime(const string& query) {
        shared_lock lock(m);
        return request_time[query];
    }
};


#endif //MULTI_THREADED_WEB_SERVER_CACHE_H
