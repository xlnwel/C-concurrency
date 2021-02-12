#include <iostream>
#include <string>

#include "join_thread.hpp"

using namespace std;

void f(const string& s) {
    cout << this_thread::get_id() << " " << s << endl;
}
int main() {
    string s("abcdefg");
    cout << std::thread::hardware_concurrency() << endl;
    utility::JoinThread t(f, s);
}