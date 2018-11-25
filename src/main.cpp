//
// main.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016 Gabriel Foust (gfoust at harding dot edu)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <iomanip>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <vector>
#include <thread>
#include "server.hpp"
#include "request.hpp"
#include "reply.hpp"
#include "spell.hpp"
#include "work_queue.h"
#include "thread_safe_map.h"
#include <sstream>
#include <chrono>

using std::this_thread::sleep_for;
using std::move;
using std::vector;
using std::string;
using std::thread;
using std::chrono::seconds;
using std::exception;
using std::cout;
using std::endl;
using std::stringstream;
using std::chrono::system_clock;
using std::chrono::seconds;
using std::chrono::duration_cast;

struct request_entry {
    const http::server::request &req;
    http::server::reply &rep;
    http::server::done_callback done;
};

thread_safe_map<string, string> cache;
thread_safe_map<string, system_clock::time_point> cache_time;
work_queue<request_entry> request_queue;
vector<thread> worker_threads;
bool isRunning = true;

string create_response(const string &query) {
    stringstream stream;

    // Loop over spellcheck results
    bool first = true;
    stream << "[";
    for (auto &candidate : spell::spellcheck(query)) {
        if (first) {
            first = false;
        } else {
            stream << ", ";
        }
        stream << "\n  { \"word\" : \"" << candidate.word << "\",  "
               << "\"distance\" : " << candidate.distance << " }";
    }
    stream << "\n]";
    return stream.str();
}

void cache_result(string query, string response) {
    cache[query] = response;
    cache_time[query] = system_clock::now();
}

// Handle request by doing spell check on query string
// Render results as JSON
void spellcheck_request(const http::server::request &req, http::server::reply &rep) {
    // Set up reply
    rep.status = http::server::reply::status_type::ok;
    rep.headers["Content-Type"] = "application/json";

    string const &query = req.query;

    if (cache.contains(query)) {
        if (system_clock::now() < cache_time[query] + seconds(60)) {
            cout << query << " is being loaded from cache" << endl;
            rep.content << cache[query];
        } else {
            cout << query << " is in cache but expired" << endl;
            string body = create_response(query);
            rep.content << body;
            cache_result(query, body);
        }
    } else {
        cout << query << " is being calculated" << endl;
        string body = create_response(query);
        rep.content << body;
        cache_result(query, body);
    }
}

void dump_cache(http::server::reply &rep) {
    // Loop over spellcheck results
    bool first = true;
    rep.content << "[";
    for (auto it = cache_time.begin(); it != cache_time.end(); ++it) {
        if (first) {
            first = false;
        } else {
            rep.content << ", ";
        }
        std::chrono::time_point time_point = it->second;
        rep.content << "\n  { \"word\" : \"" << it->first << "\",  "
                    << "\"time\" : " << std::chrono::system_clock::to_time_t(time_point) << " }";
    }
    rep.content << "\n]";
}

// Called by server whenever a request is received
// Must fill in reply, then call done()
void handle_request(const http::server::request &req, http::server::reply &rep, http::server::done_callback done) {
    std::cout << req.method << ' ' << req.uri << std::endl;
    request_entry entry = {
            req,
            rep,
            move(done)
    };
    request_queue.push(move(entry));
}

void worker(int id) {
    while (isRunning) {
        request_queue.waitForElement();
        if (!isRunning) {
            break;
        }

        cout << "Processing request with worker " << id << endl;

        request_entry entry = request_queue.pop();
//        sleep_for(seconds(5));

//        cout << "Done sleeping " << id << endl;

        auto &req = entry.req;
        auto &rep = entry.rep;
        auto done = entry.done;

        if (req.path == "/spell") {
            spellcheck_request(req, rep);
            done();
        } else if (req.path == "/cachedump") {
            dump_cache(rep);
            done();
        } else {
            rep = http::server::reply::stock_reply(http::server::reply::not_found);
            done();
        }
    }
}

long getNumberOfThreads() {
    return thread::hardware_concurrency() * 2;
}

void cache_worker() {
    while (isRunning) {
        sleep_for(seconds(60));
        cout << "Cleaning cache" << endl;
        for (auto it = cache_time.begin(); it != cache_time.end(); ++it) {
            if (system_clock::now() > it->second + seconds(60)) {
                cout << "Pruning " << it->first << endl;
                cache_time.erase(it->first);
                cache.erase(it->first);
            }
        }
    }
}

void init_workers() {
    long numberOfWorkerThreads = getNumberOfThreads();
    for (int i = 0; i < numberOfWorkerThreads; i++) {
        thread t(worker, i);
        worker_threads.emplace_back(move(t));
    }
    thread cw(cache_worker);
    worker_threads.emplace_back(move(cw));
}

void close_workers() {
    isRunning = false;
    request_queue.stop();
    for (auto &t : worker_threads) {
        t.join();
    }
}

// Initialize and run server
int main() {
    init_workers();
    try {
        std::cout << "Listening on port 8000..." << std::endl;
        http::server::server s("localhost", "8000", handle_request);
        s.run();
    }
    catch (std::exception &e) {
        std::cerr << "exception: " << e.what() << "\n";
    }

    close_workers();

    return 0;
}