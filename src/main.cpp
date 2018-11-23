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

using std::this_thread::sleep_for;
using std::move;
using std::vector;
using std::string;
using std::thread;
using std::chrono::seconds;
using std::exception;

struct request_entry {
    const http::server::request &req;
    http::server::reply &rep;
    http::server::done_callback done;
};

work_queue<request_entry> request_queue;
vector<thread> worker_threads;
bool isRunning = true;

// Handle request by doing spell check on query string
// Render results as JSON
void spellcheck_request(const http::server::request &req, http::server::reply &rep) {
    // Set up reply
    rep.status = http::server::reply::status_type::ok;
    rep.headers["Content-Type"] = "application/json";

    // Loop over spellcheck results
    bool first = true;
    rep.content << "[";
    for (auto &candidate : spell::spellcheck(req.query)) {
        if (first) {
            first = false;
        } else {
            rep.content << ", ";
        }
        rep.content << "\n  { \"word\" : \"" << candidate.word << "\",  "
                    << "\"distance\" : " << candidate.distance << " }";
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

void worker() {
    while (isRunning) {
        request_queue.waitForElement();
        if (!isRunning) {
            break;
        }

        request_entry entry = request_queue.pop();
//		sleep_for(seconds(5));
        auto &req = entry.req;
        auto &rep = entry.rep;
        auto done = entry.done;
        if (req.path == "/spell") {
            spellcheck_request(entry.req, entry.rep);
            entry.done();
        } else {
            rep = http::server::reply::stock_reply(http::server::reply::not_found);
            done();
        }
    }
}

long getNumberOfThreads() {
    return thread::hardware_concurrency() * 2;
}

void init_workers() {
    long numberOfWorkerThreads = getNumberOfThreads();
    for (int i = 0; i < numberOfWorkerThreads; i++) {
        thread t(worker);
        worker_threads.emplace_back(move(t));
    }
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