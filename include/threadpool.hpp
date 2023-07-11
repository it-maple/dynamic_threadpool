#pragma once

#include "threadsafe_queue.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <map>
#include <future>
#include <functional>

struct threadpool_config 
{
    // permanent thread
    int core_threads;
    // will create more thread to execute task when task number for the task queue get to a threshod.
    int max_threads;
    // the timeout time for cache thread
    std::chrono::seconds timeout;
};

class threadpool
{
    public:
        threadpool(int pool_size) : shutdown_(false), available_(true), inited_(false), config_({pool_size, pool_size, std::chrono::seconds(0)}) {}
        threadpool(threadpool_config config) : shutdown_(false), available_(true), inited_(false), config_(config) 
        {
            if (config_.core_threads > config_.max_threads)
                available_ = false;
        }
        threadpool(const threadpool&) = delete;
        threadpool(threadpool&&) = delete;
        threadpool& operator=(const threadpool&) = delete;
        threadpool& operator=(threadpool&&) = delete;
        ~threadpool()
        {
            for (auto & worker : workers_)
            {
                if (worker->thread_->joinable())
                    worker->thread_->join();
            }
            for (auto & pair : cache_workers_)
            {
                if (pair.second->thread_->joinable())
                    pair.second->thread_->join();
            }

            shutdown();
        }

    private:
        enum class thread_flag { CORE, CACHE };

        using thread_ptr = std::shared_ptr<std::thread>;

        struct thread_wrapper 
        {
            thread_ptr thread_;
            thread_flag flag_;

            thread_wrapper() : thread_(), flag_(thread_flag::CORE) {}
            thread_wrapper(const thread_wrapper&) = delete;
            thread_wrapper(thread_wrapper&&) = delete;
            thread_wrapper& operator=(const thread_wrapper&) = delete;
            thread_wrapper& operator=(thread_wrapper&&) = delete;
            ~thread_wrapper()
            {
                thread_->join();
                thread_.reset();
            }
        };

        using thread_wrapper_ptr = std::shared_ptr<thread_wrapper>;

    private:
        void init();

        void thread_func();

        void thread_func_with_flag(thread_flag flag);

        void add_cache_thread();

    public:
        void reset_config(const threadpool_config& config);

        bool is_available() const { return available_; }

        void start();

        void shutdown();

        void submit(std::function<void()> func);

        template<typename F, typename... Args>
        auto async(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    private:
        bool shutdown_;
        bool available_;
        bool inited_;

        threadpool_config config_;

        std::once_flag init_once_;

        // std::vector<std::thread> workers_;
        std::vector<thread_wrapper_ptr> workers_;
        std::map<std::thread::id, thread_wrapper_ptr> cache_workers_;
        mutable threadsafe_queue_v2<std::function<void()>> tasks_;

        std::mutex mutex_;
        std::mutex init_mutex_;
        std::condition_variable cv_;
};


template<typename F, typename... Args>
inline auto threadpool::async(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
{
    {
        add_cache_thread();
    }

    std::function<decltype(f(args...))()> task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(task);

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!available_ || shutdown_)
            return task_ptr->get_future();

        std::function<void()> wrapped_task = [task_ptr] {
            (*task_ptr)();
        };

        tasks_.push(wrapped_task);
    }

    // lazy initialization for worker thread
    {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (!inited_)
            start();
    }

    cv_.notify_one();

    return task_ptr->get_future();
}