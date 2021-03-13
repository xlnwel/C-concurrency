#include <atomic>
#include <memory>
#include <string>
#include <unordered_set>
#include <iostream>

template<typename T> 
class lock_free_stack { 
private:
    struct node { 
        std::shared_ptr<T> data; 
        node* next; 
        node(T const& data_): data(std::make_shared<T>(data_)) {} 
    }; 
    std::atomic<node*> head; 
public:
    void push(T const& data) { 
        node* const new_node = new node(data); 
        new_node->next = head.load(); 
        while(!head.compare_exchange_weak(new_node->next, new_node)); 
    } 
    std::shared_ptr<T> pop() { 
        node* old_head = head.load(); 
        while(old_head 
            && !head.compare_exchange_weak(old_head, old_head->next)); 
        
        auto res = old_head ? old_head->data : std::shared_ptr<T>();
        if (old_head)
            delete old_head;
        return res;
    }
};

struct Node {
    int v;
    Node* next;
};

class A {
public:
    A() {}
    A(const A&) { std::cout << "copy\n";}
    A(A&&) { std::cout << "move\n";}
};

A f(A&& a) {
    return std::move(a);
}
int main() {
    // std::atomic head = new Node();
    // auto old_node = head.load();
    // old_node->next = new Node();
    // head.exchange(old_node->next);
    // delete old_node;
    // std::cout << bool(old_node) << '\n';
    // head.compare_exchange_weak(old_node, old_node->next);
    // std::cout << old_node->v;
    // std::shared_ptr<Node> p = nullptr;
    // std::cout << bool(p);
    // A a;
    // f(A(a));
    // std::atomic<std::shared_ptr<int>> p;
    // std::cout << p.compare_exchange_weak(nullptr, 3) << *p.load();
    std::bitset<3> a("101");
    std::bitset<3> b("100");
    std::cout << (a | b.flip()).all() << std::endl;
}