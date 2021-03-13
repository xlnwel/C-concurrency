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

        template<typename... Args, 
            typename ReturnType=typename std::result_of<std::decay_t<Func>(std::decay_t<Args>...)>::type>
        std::future<ReturnType> submit(Func f, bool local=true);
    private:
        void worker_thread();
        void run_task();
        using LocalThreadType = std::queue<std::packaged_task<Func>>;
        static thread_local LocalThreadType local_queue;   // local queue, not used for now
        using ThreadSafeQueue = LockBasedQueue<std::packaged_task<int()>, 
                                    std::list<std::packaged_task<int()>>>;
        std::shared_ptr<ThreadSafeQueue> shared_queue;
        std::atomic_bool done;
        std::vector<JoinThread> threads;
    };

    template<typename Func>
    ThreadPool<Func>::ThreadPool(std::size_t n): shared_queue(new ThreadSafeQueue{}), done(false) {
        threads.emplace_back(&ThreadPool::worker_thread, this);
    }

    template<typename Func>
    ThreadPool<Func>::~ThreadPool() {
        done.store(true, std::memory_order_relaxed);
    }

    template<typename Func>
    template<typename...Args, 
        typename ReturnType>
    std::future<ReturnType> ThreadPool<Func>::submit(Func f, bool local) {
        auto result = local? post_task(f, local_queue):
            post_task(f, *shared_queue);
        return result;
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