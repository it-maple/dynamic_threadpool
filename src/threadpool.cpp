#include "threadpool.hpp"

void threadpool::init()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!available_ || shutdown_)
        return;

    // wait for task to enqueue
    cv_.wait(lock, [this] {
        return !tasks_.empty();
    });

    for (int i = 0; i < config_.core_threads; i++)
    {
        // workers_.emplace_back(std::bind(&threadpool::thread_func, this));

        thread_wrapper_ptr wrapped_thread_ptr;
        wrapped_thread_ptr->thread_ = std::make_shared<std::thread>(std::bind(&threadpool::thread_func, this));
        wrapped_thread_ptr->flag_.store(thread_flag::CORE);
        workers_.emplace_back(std::move(wrapped_thread_ptr));
    }
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
    if (size >= (config_.core_threads << 5))
    {
        for (int i = config_.core_threads; i < config_.max_threads; i++) 
        {
            thread_wrapper wrapped_thread;
            wrapped_thread.thread_ = std::make_shared<std::thread>(std::bind(&threadpool::thread_func, this));
            wrapped_thread.flag_.store(thread_flag::CACHE);
            workers_.emplace_back(std::move(wrapped_thread));
        }
    }
}

void threadpool::reset_config(const threadpool_config& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::call_once(reset_once_, [this, &config] {
        config_.core_threads = config.core_threads;
        config_.max_threads = config.max_threads;
        config_.timeout = config.timeout;
        available_ = true;
    });
}

void threadpool::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
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

    std::lock_guard<std::mutex> lock(mutex_);

    if (!available_ || shutdown_)
        return;


    tasks_.push(func);   

    cv_.notify_one();
}


template<typename F, typename... Args>
auto threadpool::async(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
{
    {
        add_cache_thread();
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!available_ || shutdown_)
        return;

    std::function<decltype(f(args...))()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(task);

    std::function<void()> wrapped_task = [task_ptr] {
        (*task_ptr)();
    };

    tasks_.push(wrapped_task);

    return task_ptr->get_future();
}