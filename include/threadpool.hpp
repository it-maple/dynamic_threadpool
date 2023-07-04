#pragma once

#include "threadsafe_queue.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
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
        threadpool(int pool_size) : shutdown_(false), available_(true), config_({pool_size, pool_size, std::chrono::seconds(0)}) {}
        threadpool(threadpool_config config) : shutdown_(false), available_(true), config_(config) 
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

            shutdown();
        }

    private:
        void init();

        void thread_func();

        void add_cache_thread();

    public:
        void reset_config(const threadpool_config& config);

        void start();

        void shutdown();

        void submit(std::function<void()> func);

        template<typename F, typename... Args>
        auto async(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    private:
        enum class thread_flag { CORE, CACHE };

        using thread_ptr = std::shared_ptr<std::thread>;
        using thread_flag_atomic = std::atomic<thread_flag>;

        struct thread_wrapper 
        {
            thread_ptr thread_;
            thread_flag_atomic flag_;

            thread_wrapper() : thread_(nullptr), flag_(thread_flag::CORE) {}
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
        bool shutdown_;
        bool available_;

        threadpool_config config_;

        std::once_flag init_once_;
        std::once_flag reset_once_;

        // std::vector<std::thread> workers_;
        std::vector<thread_wrapper_ptr> workers_;
        mutable threadsafe_queue_v2<std::function<void()>> tasks_;

        std::mutex mutex_;
        std::condition_variable cv_;
};