#include <iostream>
#include <functional>
#include <future>

#include "thread_pool.hpp"


using namespace std;
using namespace utility;


double task() {
    using namespace std::chrono_literals;
    static std::atomic<int> i = 1;
    std::cout << i.fetch_add(1, std::memory_order_relaxed) << " task\n";
    return i.load(std::memory_order_relaxed);
}

int main() {
    ThreadPool<double()> thread_pool(2);
    int k = 1;
    vector<future<double>> v;
    for (auto i = 0; i != 10; ++i) {
        v.push_back(thread_pool.submit(task));
    }
    for (auto& f: v)
        cout << f.get() << '\n';
}
