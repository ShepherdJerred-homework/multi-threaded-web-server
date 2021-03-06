
#ifndef MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H
#define MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H

#include <mutex>
#include <queue>

using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::queue;
using std::condition_variable;
using std::adopt_lock;

template<typename T>
class work_queue {
private:
    mutex queueMutex;
    condition_variable doesQueueHaveElements;
    queue<T> queue;
    bool isRunning = true;

public:
    void stop() {
        isRunning = false;
        doesQueueHaveElements.notify_all();
    }

    void push(T &&e) {
        lock_guard lock(queueMutex);
        queue.emplace(e);
        doesQueueHaveElements.notify_one();
    }

    void waitForElement() {
        unique_lock lock(queueMutex);

        while (queue.size() == 0 && isRunning) {
            doesQueueHaveElements.wait(lock);
        }

        if (isRunning) {
            lock.release();
        }
    }

    T pop() {
        lock_guard lock(queueMutex, adopt_lock);
        T&& e = move(queue.front());
        queue.pop();
        return e;
    }
};


#endif //MULTI_THREADED_WEB_SERVER_WORK_QUEUE_H
