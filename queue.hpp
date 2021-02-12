#ifndef CONCURRENCY_QUEUE_H_
#define CONCURRENCY_QUEUE_H_

#include <mutex>
#include <list>
#include <deque>
#include <queue>
#include <memory>
#include <condition_variable>

namespace utility{
    template<typename T, typename Container=std::deque<T>>
    class ThreadSafeQueue {
    public:
        // constructors
        explicit ThreadSafeQueue(const Container& c): data_queue(c) {}
        explicit ThreadSafeQueue(Container&& c=Container()): data_queue(std::move(c)) {}
        template<typename Alloc> 
        explicit ThreadSafeQueue(const Alloc& a): data_queue(a) {}
        template<typename Alloc> 
        ThreadSafeQueue(const Container& c, const Alloc& a): data_queue(c, a) {}
        template<typename Alloc> 
        ThreadSafeQueue(Container&& c, const Alloc& a): data_queue(std::move(c), a) {}
        ThreadSafeQueue(ThreadSafeQueue&& other) {
            std::scoped_lock l(m, other.m);
            data_queue(std::move(other.data_queue));
        }
        ThreadSafeQueue(const ThreadSafeQueue& other) {
            std::scoped_lock l(m, other.m);
            data_queue(other.data_queue);
        }

        template<typename Alloc>
        ThreadSafeQueue(ThreadSafeQueue&& other, const Alloc& a) {
            std::scoped_lock l(other.m);
            data_queue(std::move(other.data_queue), a);
        }

        // assignments
        ThreadSafeQueue& operator=(ThreadSafeQueue&& rhs) {
            std::scoped_lock l(m, rhs.m);
            data_queue = std::move(rhs.data_queue); 
        }
        ThreadSafeQueue& operator=(const ThreadSafeQueue& rhs) { 
            std::scoped_lock l(m, rhs.m);
            data_queue = rhs.data_queue; 
        }

        // general purpose operations
        void swap(ThreadSafeQueue& other) {
            std::scoped_lock l(m, other.m);
            data_queue.swap(other.data_queue);
        }
        bool empty() const {
            std::lock_guard l(m);
            return data_queue.empty();
        }
        std::size_t size() const {
            std::lock_guard l(m);
            return data_queue.size();
        }

        // queue operations
        void push(const T&);
        void push(T&&);
        template <typename... Args>
        void emplace(Args&&... args);
        T pop();
        bool try_pop(T&);
        // delete front() and back(), these functions may waste notifications. To enable these function, one should replace notify_one() with notify_all() in push() and emplace()
        T& front() = delete;
        const T& front() const = delete;
        T& back() = delete;
        const T& back() const = delete;
    private:
        mutable std::mutex m;
        std::queue<T, Container> data_queue;
        std::condition_variable data_cond;
    };

    template<typename T, typename Container>
    void ThreadSafeQueue<T, Container>::push(const T& data) {
        std::lock_guard l(m);
        data_queue.push(data);
        data_cond.notify_one();
    }

    template<typename T, typename Container>
    void ThreadSafeQueue<T, Container>::push(T&& data) {
        std::lock_guard l(m);
        data_queue.push(std::move(data));
        data_cond.notify_one();
    }

    template<typename T, typename Container>
    template<typename... Args>
    void ThreadSafeQueue<T, Container>::emplace(Args&&... args) {
        std::lock_guard l(m);
        data_queue.emplace(std::forward<Args>(args)...);
        data_cond.notify_one();
    }

    template<typename T, typename Container>
    T ThreadSafeQueue<T, Container>::pop() {
        std::unique_lock l(m);
        data_cond.wait(l, [this]{ return !data_queue.empty(); });
        auto data = std::move(data_queue.front());
        data_queue.pop();
        return data;
    }

    template<typename T, typename Container>
    bool ThreadSafeQueue<T, Container>::try_pop(T& data) {
        std::lock_guard l(m);
        if (data_queue.empty())
            return false;
        data = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    // template<typename T, typename Container>
    // T& ThreadSafeQueue<T, Container>::front() {
    //     std::unique_lock l(m);
    //     data_cond.wait(l, [this]{ return !data_queue.empty(); });
    //     return data_queue.front();
    // }

    // template<typename T, typename Container>
    // const T& ThreadSafeQueue<T, Container>::front() const {
    //     std::unique_lock l(m);
    //     data_cond.wait(l, [this]{ return !data_queue.empty(); });
    //     return data_queue.front();
    // }

    // template<typename T, typename Container>
    // T& ThreadSafeQueue<T, Container>::back() {
    //     std::unique_lock l(m);
    //     data_cond.wait(l, [this]{ return !data_queue.empty(); });
    //     return data_queue.back();
    // }

    // template<typename T, typename Container>
    // const T& ThreadSafeQueue<T, Container>::back() const {
    //     std::unique_lock l(m);
    //     data_cond.wait(l, [this]{ return !data_queue.empty(); });
    //     return data_queue.back();
    // }

    template<typename T>
    class ThreadSafeQueue<T, std::list<T>> {
    public:
        // constructors
        ThreadSafeQueue(): head(new Node), tail(head.get()) {}
        ThreadSafeQueue(ThreadSafeQueue&&);

        // assignments
        ThreadSafeQueue& operator=(ThreadSafeQueue&&);

        // general purpose operations
        void swap(ThreadSafeQueue&);
        bool empty() const;
        std::size_t size() const;

        // queue operations
        void push(const T&);
        void push(T&&);
        template <typename... Args>
        void emplace(Args&&... args);
        T pop();
        bool try_pop(T&);
        // delete front() and back(), these functions may waste notifications. To enable these function, one should replace notify_one() with notify_all() in push() and emplace()
        T& front() = delete;
        const T& front() const = delete;
        T& back() = delete;
        const T& back() const = delete;
    private:
        struct Node {
            std::unique_ptr<T> data;
            std::unique_ptr<Node> next;
        };

        Node* get_tail() {
            std::lock_guard l(tail_mutex);
            return tail;
        }
        Node* get_head() {
            std::lock_guard l(head_mutex);
            return head.get();
        }
        std::unique_lock<std::mutex> get_head_lock() {
            std::unique_lock l(head_mutex);
            data_cond.wait(l, [this] { return head.get() != get_tail(); });
            return l;
        }
        T pop_data() {
            auto data = std::move(*head->data);
            head = std::move(head->next);
            return data;
        }

        std::unique_ptr<Node> head;
        std::mutex head_mutex;
        Node* tail;
        std::mutex tail_mutex;
        std::condition_variable data_cond;
    };

    template<typename T>
    ThreadSafeQueue<T, std::list<T>>::ThreadSafeQueue(
        ThreadSafeQueue<T, std::list<T>>&& other) {
        {
            std::scoped_lock l(head_mutex, other.head_mutex);
            head(std::move(other.data_queue));
        }
        {
            std::lock_guard l(tail_mutex);
            tail = head.get();
        }
        {
            std::lock_guard l(other.tail_mutex);
            other.tail = nullptr;
        }
    }

    template<typename T>
    ThreadSafeQueue<T, std::list<T>>& 
    ThreadSafeQueue<T, std::list<T>>::operator=(
        ThreadSafeQueue<T, std::list<T>>&& rhs) {
        {
            std::scoped_lock l(head_mutex, rhs.head_mutex);
            head(std::move(rhs.data_queue));
        }
        {
            std::lock_guard l(tail_mutex);
            tail = head.get();
        }
        {
            std::lock_guard l(rhs.tail_mutex);
            rhs.tail = nullptr;
        }
    }

    template<typename T>
    void ThreadSafeQueue<T, std::list<T>>::swap(
        ThreadSafeQueue<T, std::list<T>>& other) {
        {
            std::scoped_lock l(head_mutex, other.head_mutex);
            head(std::move(other.data_queue));
        }
        {
            std::lock_guard l(tail_mutex);
            tail = head.get();
        }
        {
            std::lock_guard l(other.tail_mutex);
            other.tail = other.head.get();
        }
    }

    template<typename T>
    inline bool ThreadSafeQueue<T, std::list<T>>::empty() const {
        return get_head() == get_tail();
    }

    template<typename T>
    std::size_t ThreadSafeQueue<T, std::list<T>>::size() const {
        int n = 0;
        std::lock_guard l(tail_mutex);  // do not use get_tail() here to avoid race condition
        for (auto p = get_head(); p != tail; p = p->next.get())
            ++n;
        return n;
    }

    template<typename T>
    void ThreadSafeQueue<T, std::list<T>>::push(const T& data) {
        {
            auto p = std::make_unique<Node>();
            std::lock_guard l(tail_mutex);
            tail->data = std::make_unique<T>(data);     // we add data to the current tail, this allows us to move head to the next when popping
            tail->next = std::move(p);
            tail = tail->next.get();
        }
        data_cond.notify_one();
    }

    template<typename T>
    void ThreadSafeQueue<T, std::list<T>>::push(T&& data) {
        {
            auto p = std::make_unique<Node>();
            std::lock_guard l(tail_mutex);
            tail->data = std::make_unique<T>(std::move(data));   // we add data to the current tail, this allows us to move head to the next when popping
            tail->next = std::move(p);
            tail = tail->next.get();
        }
        data_cond.notify_one();
    }

    template<typename T>
    template<typename...Args>
    void ThreadSafeQueue<T, std::list<T>>::emplace(Args&&... args) {
        {
            std::unique_ptr p(new Node);
            std::lock_guard l(tail_mutex);
            tail->data = std::make_unique<T>(std::forward<Args>(args)...);
            tail->next = std::move(p);
            tail = tail->next.get();
        }
        data_cond.notify_one();
    }

    template<typename T>
    T ThreadSafeQueue<T, std::list<T>>::pop() {
        auto l(get_head_lock());
        return pop_data();
    }

    template<typename T>
    bool ThreadSafeQueue<T, std::list<T>>::try_pop(T& data) {
        std::lock_guard l(head_mutex);
        if (head.get() == get_tail())
            return false;
        data = pop_data();
        return true;
    }
}

#endif