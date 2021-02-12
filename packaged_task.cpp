#include <iostream>
#include <functional>
#include <future>
#include "queue.hpp"
#include "join_thread.hpp"


// using namespace std;
using namespace utility;


/* A reimplementation of List 4.9 */
ThreadSafeQueue<std::packaged_task<int()>, std::list<std::packaged_task<int()>>> task_queue;	// thread safe queue, which handles locks inside
void task_execution_thread() {
    bool x = true;
    while (x) { // for debugging purpose, we only execute this loop once
        auto task = task_queue.pop();	// Returns the front task and removes it from queue. Waits if task_queue is empty
        task();	// execute task
        x = false;
    }
}

template<typename ReturnType, typename... Args>
std::future<ReturnType> post_task(std::function<ReturnType(Args...)> f) {
    std::packaged_task<ReturnType(Args...)> task(f);
    std::future res = task.get_future();
    task_queue.push(std::move(task));   // packaged_task is not copyable
    return res;
}

int task() {
    std::cout << "f\n";
    return 1;
}
int main() {
    JoinThread t(task_execution_thread);
    auto f = post_task(std::function<int()>(task));
    std::cout << f.get() << '\n';
}
