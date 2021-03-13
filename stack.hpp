#ifndef CONCURRENCY_STACK_H_
#define CONCURRENCY_STACK_H_

#include <mutex>
#include <memory>
#include <future>
#include <atomic>

namespace utility{
    template<typename T, typename PtrType=void*>
    class LockFreeStack {
    public:
        LockFreeStack(const LockFreeStack&) = delete;
        LockFreeStack& operator=(const LockFreeStack&) = delete;
        ~LockFreeStack();

        void push(const T& data);
        std::shared_ptr<T> pop();
    private:
        struct Node {
            std::shared_ptr<T> data;
            Node* next;
            Node(const T& d): data(std::make_shared<T>(data)) {}
        };
        std::atomic<Node*> head;
    };

    template<typename T, typename PtrType>
    LockFreeStack<T, PtrType>::~LockFreeStack() {
        auto old_head = head.load(std::memory_order_relaxed);
        while (old_head
            && head.compare_exchange_weak(old_head, old_head->next,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            delete old_head;
        }
    }
    template<typename T, typename PtrType>
    void LockFreeStack<T, PtrType>::push(const T& data) {
        const Node* p = new Node(data);
        p->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(p->next, p,
            std::memory_order_release, std::memory_order_relaxed));
    }

    template<typename T, typename PtrType>
    std::shared_ptr<T> LockFreeStack<T, PtrType>::pop() {
        auto old_head = head.load();
        while (old_head
            && head.compare_exchange_weak(old_head, old_head->next,
                std::memory_order_acquire, std::memory_order_release));
        // NOTE: it's safe to delete right now as long as we do not 
        // define any other operations that access the stack
        if (old_head) {
            auto res = old_head->data;
            delete old_head;
            return res;
        }
        return {};
    }

    /* Specialization for atomic<shared_ptr> */
    template<typename T>
    class LockFreeStack<T, std::atomic<std::shared_ptr<T>>> {
    public:
        LockFreeStack(const LockFreeStack&) = delete;
        LockFreeStack& operator=(const LockFreeStack&) = delete;
        ~LockFreeStack();

        void push(const T& data);
        std::shared_ptr<T> pop();
    private:
        struct Node {
            std::shared_ptr<T> data;
            std::shared_ptr<Node> next;
            Node(const T& d): data(std::make_shared<T>(data)) {}
        };
        std::atomic<std::shared_ptr<Node>> head;
    };

    template<typename T>
    LockFreeStack<T, std::atomic<std::shared_ptr<T>>>::~LockFreeStack() {
        auto old_head = head.load(std::memory_order_relaxed);
        while (old_head
            && head.compare_exchange_weak(old_head, old_head->next,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            // regarding that old_head might still be held by some 
            // other thread, we clear its next so that the next can
            // be reclaimed first
            old_head->next = nullptr;
        }
    }
    template<typename T>
    void LockFreeStack<T, std::atomic<std::shared_ptr<T>>>::push(const T& data) {
        auto p = std::make_shared<Node>(data);
        p->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(p->next, p,
            std::memory_order_release, std::memory_order_relaxed));
    }

    template<typename T>
    std::shared_ptr<T> 
    LockFreeStack<T, std::atomic<std::shared_ptr<T>>>::pop() {
        auto old_head = head.load();
        while (old_head
            && head.compare_exchange_weak(old_head, old_head->next,
                std::memory_order_acquire, std::memory_order_release));
        // NOTE: it's safe to delete right now as long as we do not 
        // define any other operations that access the stack
        if (old_head) {
            // regarding that old_head might still be held by some 
            // other thread, we clear its next so that the next can
            // be reclaimed first
            old_head->next = nullptr;
            return old_head->data;
        }
        return {};
    }
}

#endif
