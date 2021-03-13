#include <iostream>
#include <functional>
#include <future>
#include "queue.hpp"
#include "join_thread.hpp"


// using namespace std;
using namespace utility;


/* A reimplementation of List 4.9 */
LockBasedQueue<std::packaged_task<int()>, std::list<std::packaged_task<int()>>> task_queue;	// thread safe queue, which handles locks inside
void task_execution_thread() {
    bool x = true;
    while (x) { // for debugging purpose, we only execute this loop once
        auto task = task_queue.pop();	// Returns the front task and removes it from queue. Waits if task_queue is empty
        task();	// execute task
        x = false;
    }
}

int task() {
    static std::atomic<int> i = 1;
    std::cout << i.fetch_add(1, std::memory_order_relaxed) << "task\n";
    return i.load(std::memory_order_relaxed);
}

int main() {
    JoinThread t1(task_execution_thread);
    JoinThread t2(task_execution_thread);
    auto f1 = post_task(task, task_queue);
    auto f2 = post_task(task, task_queue);
    std::cout << "f1: " << f1.get() << '\n';
    std::cout << "f2: " << f2.get() << '\n';
}
