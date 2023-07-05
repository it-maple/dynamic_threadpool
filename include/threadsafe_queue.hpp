#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

// /**
//  * @brief implementation for thread-safe queue with coarse-grained lock on the entire queue.
//  * 
//  * @tparam T 
//  */
// template <typename T>
// class threadsafe_queue_v1
// {
//     public:
//         threadsafe_queue_v1() = default;
//         threadsafe_queue_v1(const threadsafe_queue_v1&) = delete;
//         threadsafe_queue_v1(threadsafe_queue_v1&&) = delete;
//         threadsafe_queue_v1& operator=(const threadsafe_queue_v1&) = delete;
//         threadsafe_queue_v1& operator=(threadsafe_queue_v1&&) = delete;
//         ~threadsafe_queue_v1() = default;

//     public:
//         bool empty() const;

//         size_t size() const;

//     public:
//         void push(const T& obj);

//         template<typename... Args>
//         void emplace(Args&&... args);

//         std::shared_ptr<T> wait_and_pop();

//     private:
//         mutable std::mutex mx_;
//         std::condition_variable data_cond_;
//         std::queue<T> data_queue_;
// };


/**
 * @brief implementation for thread-safe queue with fine-grained lock on the entire queue.  * 
 * @tparam T 
 */
template<typename T>
class threadsafe_queue_v2
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            std::unique_ptr<node> next;
        };

    public:
        threadsafe_queue_v2() : size_(0), head_(new node), tail_(head_.get()) {}
        threadsafe_queue_v2(const threadsafe_queue_v2&) = delete;
        threadsafe_queue_v2(threadsafe_queue_v2&&) = delete;
        threadsafe_queue_v2& operator=(const threadsafe_queue_v2&) = delete;
        threadsafe_queue_v2& operator=(threadsafe_queue_v2&&) = delete;
        ~threadsafe_queue_v2() { clear(); }

    public:
        bool empty() const
        {
	        std::lock_guard<std::mutex> head_lock(head_mutex_);
	        return (head_.get()) == get_tail();
        }

        node* get_tail() const
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex_);
            return tail_;
        }

    public:
        size_t get_size() const 
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex_);
            return size_; 
        }

        void clear()
        {
	        std::lock_guard<std::mutex> head_lock(head_mutex_);
	        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
	        std::unique_ptr<node> head;
	        head.swap(head_);
	        tail_ = head_.get();
        }

        void push(const T& obj)
        {
	        std::unique_ptr<node> new_node(new node);
	        node* new_tail = new_node.get();

	        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
	        tail_->data = std::make_shared<T>(obj);
	        tail_->next = std::move(new_node);
	        tail_ = new_tail;
	        size_++;
	        cv_.notify_one();
        }

        std::shared_ptr<T> wait_and_pop()
        {
	        std::unique_lock<std::mutex> head_lock(head_mutex_);
	        cv_.wait(head_lock, [this] { return !(head_.get() == get_tail()); });

	        auto data = head_->data;
	        std::unique_ptr<node> new_head = std::move(head_->next);
	        std::unique_ptr<node> old_head = std::move(head_);
	        head_ = std::move(new_head);
	        size_--;
	        head_lock.unlock();

	        return std::move(data);
        }

    private:
        mutable size_t size_;

        mutable std::mutex head_mutex_;
        mutable std::mutex tail_mutex_;
        std::condition_variable cv_;

        std::unique_ptr<node> head_;
        node* tail_;
};