#include <chrono>
#include <iostream>
#include "list.hpp"
#include "join_thread.hpp"

using namespace utility;
using namespace std::chrono_literals;

LockBasedList<int> tslist;


void f() {
    for (auto i = 0; i != 10; ++i) {
        tslist.push_front(i);
        std::this_thread::sleep_for(10ms);
    }
    tslist.remove_if([](int i){ return i % 2 == 0; });
    tslist.for_each([](int& i){ return i *= 2; });
}
int main() {
    utility::JoinThread t(f);
    std::this_thread::sleep_for(500ms);
    tslist.for_each([](int i){ std::cout << i << " ";});
}