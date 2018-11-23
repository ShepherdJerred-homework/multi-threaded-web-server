
#ifndef MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H
#define MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H

#include <mutex>
#include <queue>

using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::queue;
using std::condition_variable;

template<typename T>
class work_queue {
private:
    mutex m;
    condition_variable c;
    queue<T> q;

public:
    void enqueue(T&& e) {
        lock_guard lock(m);
        q.emplace(e);
        c.notify_one();
    }

    T dequeue() {
        unique_lock lock(m);

        while (q.size() == 0) {
            c.wait(lock);
        }

        T&& e = move(q.front());
        q.pop();
        return e;
    }
};


#endif //MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H
