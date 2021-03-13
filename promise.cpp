#include <iostream>
#include <future>
#include <thread>

void working_thread(std::promise<bool> p) {
    std::cout << "do some work\n";
    p.set_value(true);
}


int main() {
    std::promise<bool> done_promise;
    auto done_future = done_promise.get_future();
    std::thread t(working_thread, std::move(done_promise));
    done_future.get();
    t.join();
}
