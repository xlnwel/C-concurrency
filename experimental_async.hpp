#ifndef CONCURRENCY_EXPERIMENTAL_ASYNC_H_
#define CONCURRENCY_EXPERIMENTAL_ASYNC_H_

#include <experimental/future>


namespace utility {
    """ Async that returns std::experimental::future """
    template<typename Function, typename... Args, 
        typename ReturnType=typename std::result_of<std::decay_t<Function>(std::decay_t<Args>...)>::type>
    std::experimental::future<ReturnType> experimental_async(Function&& func, Args&&... args){
        using ReturnType = ;
        std::experimental::promise<ReturnType> p; 
        auto res=p.get_future(); 
        std::thread t(
            [p=std::move(p),f=std::decay_t<Function>(func)]() mutable {
                try{ 
                    p.set_value_at_thread_exit(f()); 
                } catch(...) { 
                    p.set_exception_at_thread_exit(std::current_exception()); 
            }
        }, std::forward<Args>(args)...); 
        t.detach(); 
        return res;
    }
}

#endif