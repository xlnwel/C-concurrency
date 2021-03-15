#include <atomic>
#include <functional>
#include <future>
#include <iostream>

#include "queue.hpp"
#include "join_thread.hpp"


using namespace std;
using namespace utility;


/* A reimplementation of List 4.9 */
using Func = double();
atomic<bool> go;
LockBasedQueue<std::packaged_task<Func>, std::list<std::packaged_task<Func>>> task_queue;	// thread safe queue, which handles locks inside
void task_execution_thread() {
    while (!go.load())
        this_thread::yield();
    packaged_task<double()> task;
    while (!task_queue.empty()) { // for debugging purpose, we only execute this loop once
        auto flag = task_queue.try_pop(task);	// Returns the front task and removes it from queue. Waits if task_queue is empty
        if (flag)
            task();	// execute task
        else {
            cout << this_thread::get_id() << "yield\n";
            this_thread::yield();
        }
    }
}

double task(int n=1) {
    static std::atomic<int> i = 1;
    std::cout << i.fetch_add(1, std::memory_order_relaxed) << "task\n";
    return i.load(std::memory_order_relaxed);
}

int main() {
    go = false;
    JoinThread t1(task_execution_thread);
    JoinThread t2(task_execution_thread);
    // auto f1 = post_task(task_queue, task, 3);
    // auto f2 = post_task(task_queue, task, 5);
    // std::cout << "f1: " << f1.get() << '\n';
    // std::cout << "f2: " << f2.get() << '\n';
    vector<future<double>> v;
    for (auto i = 0; i != 10; ++i) {
        v.push_back(post_task(task_queue, task, i));
    }
    go = true;
    for (auto& f: v)
        cout << f.get() << '\n';
}
