#include "threadpool.hpp"
#include <mutex>

void threadpool::init()
{
    if (!available_ || shutdown_)
        return;

    for (int i = 0; i < config_.core_threads; i++)
    {
        thread_wrapper_ptr wrapped_thread_ptr = std::make_shared<thread_wrapper>();
        wrapped_thread_ptr->thread_ = std::make_shared<std::thread>(std::bind(&threadpool::thread_func, this));
        wrapped_thread_ptr->flag_.store(thread_flag::CORE);
        workers_.push_back(std::move(wrapped_thread_ptr));
    }

    inited_ = true;
}

void threadpool::thread_func()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!available_ || shutdown_)
            return;

        cv_.wait(lock, [this] {
            return !tasks_.empty();
        });

        if (!shutdown_)
        {
            auto task = tasks_.wait_and_pop();
            (*task)();
        }
    }
}

void threadpool::add_cache_thread()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!available_ || shutdown_)
        return;

    auto size = tasks_.get_size();
    // (a << b) ==> (a * 2 ^ b)
    if (size >= static_cast<size_t>(config_.core_threads << 5))
    {
        for (int i = config_.core_threads; i < config_.max_threads; i++) 
        {
            thread_wrapper_ptr wrapped_thread_ptr;
            wrapped_thread_ptr->thread_ = std::make_shared<std::thread>(std::bind(&threadpool::thread_func, this));
            wrapped_thread_ptr->flag_.store(thread_flag::CORE);
            workers_.push_back(std::move(wrapped_thread_ptr));
        }
    }
}

void threadpool::reset_config(const threadpool_config& config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    config_.core_threads = config.core_threads;
    config_.max_threads = config.max_threads;
    config_.timeout = config.timeout;

    if (config.core_threads > config.max_threads)
        available_ = false;
    else
        available_ = true;
}

void threadpool::start()
{
    // wait for task to enqueue
    if (tasks_.get_size() == 0)
        return;

    std::call_once(init_once_, [this] { init(); });
}

void threadpool::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!available_ || shutdown_)
        return;

    shutdown_ = true;

    cv_.notify_all();

    tasks_.clear();
}

void threadpool::submit(std::function<void()> func)
{
    {
        add_cache_thread();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!available_ || shutdown_)
            return;

        tasks_.push(func);   
    }

    // lazy initialization for worker thread
    {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (!inited_)
            start();
    }

    cv_.notify_one();
}