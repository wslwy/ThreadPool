// ThreadPool.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。
// C++14 实现

#pragma once

#include <iostream>

// TODO: 在此处引用程序需要的其他标头。
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

//// C++14 兼容的元组展开工具
//namespace detail {
//	template <typename F, typename Tuple, size_t... I>
//	auto invoke_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
//		return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
//	}
//}
//
//template <typename F, typename Tuple>
//auto invoke_from_tuple(F&& f, Tuple&& t) {
//	constexpr auto size = std::tuple_size<std::decay_t<Tuple>>::value;
//	return detail::invoke_impl(std::forward<F>(f), std::forward<Tuple>(t),
//		std::make_index_sequence<size>{});
//}

// ThreadPool.h: 线程池类的定义和实现
class ThreadPool {
public:
	ThreadPool(size_t num_threads);

	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args);

	~ThreadPool() noexcept;

private:
	// need to keep track of threads so we can join them
	std::vector<std::thread> workers;
	// the task queue
	std::queue<std::function<void()>> tasks;

	// synchronization
	std::mutex queue_mutex;
	std::condition_variable condition_v;
	bool stop;
};


// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t num_threads)
	: stop(false) {
	workers.reserve(num_threads); // 预分配内存

	try {
		for (size_t i = 0; i < num_threads; ++i) {
			workers.emplace_back([this]() {
				while (true) {
					// 1. 定义任务容器
					std::function<void()> task;
					{
						// 2. 通过条件变量等待任务
						std::unique_lock<std::mutex> lock(queue_mutex);
						condition_v.wait(lock, [this]() {
							return stop || !tasks.empty();
							});

						// 3. 检查终止条件
						if (stop && tasks.empty())
							return;

						// 4. 取任务
						task = std::move(tasks.front());
						tasks.pop();
					}

					// 5. 执行任务（无锁状态）
					try {
						task();
					}
					catch (const std::exception& e) {
						// 改进的异常处理
						std::cerr << "ThreadPool exception: " << e.what() << std::endl;
					}
					catch (...) {
						std::cerr << "Unknown exception in ThreadPool" << std::endl;
					}
				}
			});
		}
	} 
	catch (...) {
		// 异常安全处理
		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition_v.notify_all();
		for (auto& worker : workers) {
			if (worker.joinable()) worker.join();
		}
		throw;
	}
};

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) {
	using task_return_type = std::result_of_t<F(Args...)>;
	//using return_type = std::future< task_return_type >;

	// 1. 使用 std::packaged_task 来包装函数和参数,统一包装成 void() 签名
	auto task = std::make_shared< std::packaged_task<task_return_type()> >(
		[func = std::forward<F>(f), 
		targs = std::make_tuple(std::forward<Args>(args)...)]() mutable {
			return std::apply(std::move(func), std::move(targs));	// C++ 17支持
		}
	);

	// 2. 获取返回值的 future
	auto res = task->get_future();
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		// don't allow enqueueing after stopping the pool
		if (stop) 
			throw std::runtime_error("enqueue on stopped ThreadPool");
		tasks.emplace([task]() { 
			(*task)(); 
		});

		//// 潜在智能通知优化
		//if (tasks.size() == 1)
		//	condition_v.notify_all();
		//else
		//	condition_v.notify_one();
	}

	// 3. 通知一个等待的线程
	condition_v.notify_one();
	//return static_cast<return_type>(res);
	return res; // 自动推导
}


// the destructor joins all threads
inline ThreadPool::~ThreadPool() noexcept
{
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		stop = true;
	}	// 设置停止标志

	condition_v.notify_all();	// 唤醒所有工作线程
	for (std::thread& worker : workers) {
		if (worker.joinable()) {
			try {
				worker.join();
			}
			catch (...) {
				// 处理异常（日志/终止等）
				std::terminate();  // 或者吞掉异常（不推荐）
			}
		}
	}
};
