#include <iostream>
#include <functional>
#include <future>

#include "thread_pool.hpp"


// using namespace std;
using namespace utility;


int task() {
    static std::atomic<int> i = 1;
    std::cout << i.fetch_add(1, std::memory_order_relaxed) << "task\n";
    return i.load(std::memory_order_relaxed);
}

int main() {
    ThreadPool<int()> thread_pool(1);
    auto f1 = thread_pool.submit(task, false);
    auto f2 = thread_pool.submit(task, false);
    auto f3 = thread_pool.submit(task, false);
    std::cout << f1.get() << '\n';
    std::cout << f2.get() << '\n';
    std::cout << f3.get() << '\n';
}
