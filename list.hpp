#ifndef CONCURRENCY_THREADSAFE_LIST_H_
#define CONCURRENCY_THREADSAFE_LIST_H_

#include <mutex>
#include <memory>
#include <functional>

namespace utility{
    template<typename T>
    class LockBasedList {
    public:
        LockBasedList() {}
        LockBasedList(const LockBasedList&) = delete;
        LockBasedList& operator=(const LockBasedList&) = delete;
        ~LockBasedList();

        // we use shared_ptr as Anthony did in Chapter 6.3. but
        // it's worth noting that returning shared_ptr is still
        // not safe as it's changeable and one may change the  
        // data from the outside of the scope without any 
        // protection mechanism, but it seems the best
        // option we can get without exposing Node.
        std::shared_ptr<T> front() const;
        template<typename Pred>
        std::shared_ptr<T> find_if(Pred);

        template<typename Func>
        void for_each(Func);

        void push_front(const T&);
        void push_front(T&&);
        // delete push_back to avoid the use of tail, which will incur an additional lock in push_front
        void push_back(const T&) = delete;
        void push_back(T&&) = delete;
        template<typename Pred>
        void insert_if(const T&, Pred=std::equal_to());
        template<typename Pred>
        void insert_if(T&&, Pred=std::equal_to());
        template<typename Pred>
        void remove_if(Pred);

    private:
        struct Node {
            Node() {}
            Node(const T& d, std::unique_ptr<Node>&& p={}): 
                data(std::make_shared<T>(d)), next(std::move(p)) {}
            Node(T&& d, std::unique_ptr<Node>&& p={}): 
                data(std::make_shared<T>(std::move(d))), next(std::move(p)) {}
            std::mutex m;
            std::shared_ptr<T> data;
            std::unique_ptr<Node> next;
        };
        Node head;
        // Node* tail;
    };
    
    template<typename T>
    LockBasedList<T>::~LockBasedList<T>() {
        remove_if([](const Node&){ return true;});
    }

    template<typename T>
    std::shared_ptr<T> LockBasedList<T>::front() const {
        std::lock_guard l(head);
        if (head.next)
            return head.next->data;
        else
            return {};
    }

    template<typename T>
    template<typename Pred>
    std::shared_ptr<T> LockBasedList<T>::find_if(Pred pred) {
        auto p = &head;
        std::unique_lock l(head.m);
        while ((p = p->next.get())) {
            l = std::unique_lock(p->m);
            if (pred(*p->data))
                return p->data;
        }
        return {};
    }

    template<typename T>
    template<typename Func>
    void LockBasedList<T>::for_each(Func f) {
        auto p = &head;
        std::unique_lock l(head.m);
        while ((p = p->next.get())) {
            l = std::unique_lock(p->m);
            f(*p->data);
        }
    }

    template<typename T>
    void LockBasedList<T>::push_front(const T& data) {
        auto new_node = std::make_unique<Node>(data);
        std::lock_guard l(head.m);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
        // std::lock_guard l2(tail->m);
        // if (!tail)
        //     tail = head->next.get();
    }

    template<typename T>
    void LockBasedList<T>::push_front(T&& data) {
        auto new_node = std::make_unique<Node>(std::move(data));
        std::lock_guard l(head.m);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
        // std::lock_guard l2(tail->m);
        // if (!tail)
        //     tail = head->next.get();
    }

    // template<typename T>
    // void LockBasedList<T>::push_back(const T& data) {
    //     auto new_node = std::make_unique<Node>(data);
    //     std::lock_guard l(tail->m);
    //     tail->next = std::move(new_node);
    //     tail = tail->next.get();
    // }

    // template<typename T>
    // void LockBasedList<T>::push_back(T&& data) {
    //     auto new_node = std::make_unique<Node>(std::move(data));
    //     std::lock_guard l(tail->m);
    //     tail->next = std::move(new_node);
    //     tail = tail->next.get();
    // }


    template<typename T>
    template<typename Pred>
    void LockBasedList<T>::insert_if(const T& data, Pred pred) {
        auto p = &head;         // safe as head always exists
        {
            std::unique_lock l(head.m);
            while ((p = p->next.get())) {   // this is safe as we've already locked p->m
                l = std::unique_lock(p->m);
                if (pred(data, *p->data)) {
                    p->data.reset(new T(data));
                    return;
                }
            }
        }
        push_front(data);
    }

    template<typename T>
    template<typename Pred>
    void LockBasedList<T>::insert_if(T&& data, Pred pred) {
        auto p = &head;         // safe as head always exists
        {
            std::unique_lock l(head.m);
            while ((p = p->next.get())) {   // this is safe as we've already locked p->m
                l = std::unique_lock(p->m);
                if (pred(data, *p->data)) {
                    p->data.reset(new T(std::move(data)));
                    return;
                }
            }
        }
        push_front(std::move(data));
    }

    template<typename T>
    template<typename Pred>
    void LockBasedList<T>::remove_if(Pred pred) {
        auto p = &head;         // safe as head always exists
        std::unique_lock l(head.m);
        while (auto p2 = p->next.get()) {   // this is safe as we've already locked p->m
            std::unique_lock l2(p2->m);
            if (pred(*p2->data)) {
                p->next = std::move(p2->next);
                l2.unlock();
            }
            else {
                l.unlock();
                p = p2;
                l = std::move(l2);
            }
        }
    }
}

#endif