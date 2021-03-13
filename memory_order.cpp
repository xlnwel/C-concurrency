#include <atomic>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "join_thread.hpp"

using namespace std;
using namespace utility;

atomic<bool> x;
atomic<bool> y;
atomic<int> z;
atomic<bool> go;

void write_x(memory_order mo) {
    while (!go.load())
        std::this_thread::yield();
    x.store(true, mo);
}

void write_y(memory_order mo) {
    while (!go.load())
        std::this_thread::yield();
    y.store(true, mo);
}

void write_x_then_y(memory_order mo) {
    while (!go.load())
        std::this_thread::yield();
    x.store(true, mo);
    y.store(true, mo);
}

void write_y_then_x(memory_order mo) {
    while (!go.load())
        std::this_thread::yield();
    y.store(true, mo);
    x.store(true, mo);
}

void read_x_then_y(memory_order mo) {
    while (!x.load(mo));
    if (y.load(mo))
        ++z;
}

void read_y_then_x(memory_order mo) {
    while (!y.load(mo));
    if (x.load(mo))
        ++z;
}

void reset_atomics() {
    x = false;
    y = false;
    z = 0;
    go = false;
}

void fill(memory_order mo, memory_order& write_mo, memory_order& read_mo, string& s) {
    switch(int(mo)) {
    case memory_order_seq_cst: 
        write_mo = memory_order_seq_cst;
        read_mo = memory_order_seq_cst;
        s = "seq_cst";
        break;
    case memory_order_acq_rel: 
        write_mo = memory_order_release;
        read_mo = memory_order_acquire;
        s = "acq_rel";
        break;
    case memory_order_relaxed: 
        write_mo = memory_order_relaxed;
        read_mo = memory_order_relaxed;
        s = "relaxed";
        break;
    default:
        throw domain_error("Unknown mo");
    }
}

int test_single_threading(memory_order mo) {
    reset_atomics();
    memory_order write_mo;
    memory_order read_mo;
    string s;
    fill(mo, write_mo, read_mo, s);
    vector<JoinThread> threads;
    threads.emplace_back(write_x_then_y, write_mo);
    threads.emplace_back(read_y_then_x, read_mo);
    go = true;
    threads.clear();
    cout << "Single-threading writing and reading with release and acquire:\t" << z.load() << endl;
    return z.load();
}

int test_multiple_threading(memory_order mo) {
    reset_atomics();
    memory_order write_mo;
    memory_order read_mo;
    string s;
    fill(mo, write_mo, read_mo, s);
    vector<JoinThread> threads;
    threads.emplace_back(write_x, write_mo);
    threads.emplace_back(write_y, write_mo);
    threads.emplace_back(read_x_then_y, read_mo);
    threads.emplace_back(read_y_then_x, read_mo);
    go = true;
    threads.clear();
    cout << "Multi-threading writing and reading with " + s + ":\t\t" << z.load() << endl;
    return z.load();
}

int main() {
    int n = 5;
    vector<int> zs(n);
    for (auto i = 0; i != 100; ++i) {
        zs[0] += test_single_threading(memory_order_relaxed);
        zs[1] += test_single_threading(memory_order_acq_rel);
        zs[2] += test_multiple_threading(memory_order_relaxed);
        zs[3] += test_multiple_threading(memory_order_acq_rel);
        zs[4] += test_multiple_threading(memory_order_seq_cst);
    }
    for (auto i = 0; i != n; ++i)
        cout << i << " : " << zs[i] << '\n';
}
