#ifndef CONCURRENCY_THREADPOOL_H_
#define CONCURRENCY_THREADPOOL_H_

#include <atomic>
#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>

#include "queue.hpp"
#include "join_thread.hpp"

namespace utility {
    template<typename Func>
    class ThreadPool {
    public:
        ThreadPool(std::size_t=std::thread::hardware_concurrency()); // should I minus one here for the main thread?
        ~ThreadPool();

        template<typename FuncType, typename... Args, // a separate FuncType required for lambda functions
            typename ReturnType=typename std::invoke_result<
                std::decay_t<FuncType>, std::decay_t<Args>...>::type>
        std::future<ReturnType> submit(FuncType&& f, Args&&...args);
        template<typename FuncType, typename... Args, 
            typename ReturnType=typename std::invoke_result<
                std::decay_t<FuncType>, std::decay_t<Args>...>::type>
        std::future<ReturnType> submit_local(FuncType&& f, Args&&...args);
        void stop();    // stop may delay until the current task in each thread is finished
        void restart();
    private:
        void worker_thread();
        void run_task();
        using LocalThreadType = std::queue<std::packaged_task<Func>>;
        static thread_local LocalThreadType local_queue;   // local queue, not used for now
        using SharedQueueType = LockBasedQueue<std::packaged_task<Func>, 
                                    std::list<std::packaged_task<Func>>>;
        std::shared_ptr<SharedQueueType> shared_queue;
        std::atomic_bool done;
        std::vector<JoinThread> threads;
    };

    template<typename Func>
    ThreadPool<Func>::ThreadPool(std::size_t n):
        shared_queue(new SharedQueueType{}), done(false) {
        for (auto i = 0; i != n; ++i)
            threads.emplace_back(&ThreadPool::worker_thread, this);
    }

    template<typename Func>
    ThreadPool<Func>::~ThreadPool() {
        done.store(true, std::memory_order_relaxed);
    }

    template<typename Func>
    template<typename FuncType, typename...Args, typename ReturnType>
    std::future<ReturnType> ThreadPool<Func>::submit(FuncType&& f, Args&&...args) {
        auto result = post_task(*shared_queue,
            std::forward<FuncType>(f), std::forward<Args>(args)...);
        return result;
    }

    template<typename Func>
    template<typename FuncType, typename...Args, typename ReturnType>
    std::future<ReturnType> ThreadPool<Func>::submit_local(FuncType&& f, Args&&...args) {
        auto result = post_task(local_queue, 
            std::forward<FuncType>(f), std::forward<Args>(args)...);
        return result;
    }

    template<typename Func>
    void ThreadPool<Func>::stop() {
        done.store(true, std::memory_order_relaxed);
    }

    template<typename Func>
    void ThreadPool<Func>::restart() {
        done.store(false, std::memory_order_relaxed);
    }

    template<typename Func>
    void ThreadPool<Func>::run_task() {
        if (!local_queue.empty()) {
            auto task = std::move(local_queue.front());
            local_queue.pop();
            task();
        }
        else {
            std::packaged_task<Func> task;
            auto flag = shared_queue->try_pop(task);
            if (flag)
                task();
            else {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s);
            }
        }
    }

    template<typename Func>
    void ThreadPool<Func>::worker_thread() {
        while (!done.load(std::memory_order_relaxed)) {
            run_task();
        }
    }

    template<typename Func>
    thread_local typename ThreadPool<Func>::LocalThreadType ThreadPool<Func>::local_queue = {};
}

#endif
