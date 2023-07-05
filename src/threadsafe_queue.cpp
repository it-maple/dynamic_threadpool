// #include "threadsafe_queue.hpp"
// #include <memory>
// #include <mutex>
// #include <utility>

// template<typename T>
// inline bool threadsafe_queue_v1<T>::empty() const
// {
// 	std::lock_guard<std::mutex> lock(mx_);
// 	return data_queue_.empty();
// }

// template<typename T>
// inline size_t threadsafe_queue_v1<T>::size() const
// {
// 	std::lock_guard<std::mutex> lock(mx_);
// 	return data_queue_.size();
// }

// template<typename T>
// inline void threadsafe_queue_v1<T>::push(const T &obj)
// {
// 	std::lock_guard<std::mutex> lock(mx_);
// 	data_queue_.push(obj);
// 	data_cond_.notify_one();
// }

// template<typename T>
// template<typename... Args>
// inline void threadsafe_queue_v1<T>::emplace(Args&&... args)
// {
// 	std::lock_guard<std::mutex> lock(mx_);
// 	// perfect forwarding technique
// 	// neither lvalue or rvalue depending on Args
// 	data_queue_.emplace(std::forward<Args>(args)...);
// 	data_cond_.notify_one();
// }

// template<typename T>
// inline std::shared_ptr<T> threadsafe_queue_v1<T>::wait_and_pop()
// {
// 	std::unique_lock<std::mutex> lock(mx_);
// 	data_cond_.wait(lock, [this] {return data_queue_.empty(); });
// 	std::shared_ptr<T> res(
// 		std::make_shared<T>(std::move(data_queue_.front()))
// 	);
// 	data_queue_.pop();

// 	return res;
// }



// template<typename T>
// inline bool threadsafe_queue_v2<T>::empty() const
// {
// 	std::lock_guard<std::mutex> head_lock(head_mutex_);
// 	return (head_.get()) == get_tail();
// }

// template<typename T>
// inline void threadsafe_queue_v2<T>::clear()
// {
// 	std::lock_guard<std::mutex> head_lock(head_mutex_);
// 	std::lock_guard<std::mutex> tail_lock(tail_mutex_);
// 	std::unique_ptr<node> head;
// 	head.swap(head_);
// 	tail_ = head_.get();
// }

// template<typename T>
// inline void threadsafe_queue_v2<T>::push(const T& obj)
// {
// 	std::unique_ptr<node> new_node(new node);
// 	node* new_tail = new_node.get();

// 	std::lock_guard<std::mutex> tail_lock(tail_mutex_);
// 	tail_->data = std::make_shared<T>(obj);
// 	tail_->next = std::move(new_node);
// 	tail_ = new_tail;
// 	size_++;
// 	data_cond_.notify_one();
// }

// template<typename T>
// inline std::shared_ptr<T> threadsafe_queue_v2<T>::wait_and_pop()
// {
// 	std::unique_lock<std::mutex> head_lock(head_mutex_);
// 	data_cond_.wait(head_lock, [this] { head_.get() == get_tail(); });
// 	auto data = head_->data;
// 	std::unique_ptr<T> old_head = std::move(head_);
// 	std::unique_ptr<T> new_head = std::move(head_->next);
// 	head_ = std::move(new_head);
// 	size_--;
// 	head_lock.unlock();

// 	return std::move(data);
// }
