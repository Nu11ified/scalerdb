#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

namespace scalerdb {

/**
 * @brief Thread pool wrapper for database operations
 * 
 * This class provides a simplified interface for managing worker threads
 * in database read/write operations, with built-in load balancing and
 * task queuing capabilities using standard library components.
 */
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    size_t thread_count_;

    void worker_thread() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                
                if (stop_ && tasks_.empty()) {
                    return;
                }
                
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
            }
            
            if (task) {
                task();
            }
        }
    }

public:
    /**
     * @brief Construct a ThreadPool with specified number of threads
     * @param num_threads Number of worker threads (defaults to hardware concurrency)
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) 
        : stop_(false), thread_count_(num_threads) {
        if (num_threads == 0) {
            thread_count_ = std::thread::hardware_concurrency();
        }
        
        workers_.reserve(thread_count_);
        for (size_t i = 0; i < thread_count_; ++i) {
            workers_.emplace_back(&ThreadPool::worker_thread, this);
        }
    }

    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool() {
        stop_ = true;
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Disable copy and move
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Get the number of threads in the pool
     * @return Number of worker threads
     */
    size_t getThreadCount() const {
        return thread_count_;
    }

    /**
     * @brief Submit a task for execution
     * @param task Function to execute
     * @param args Arguments for the function
     * @return Future for the result
     */
    template<typename F, typename... Args>
    auto submit(F&& task, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto packaged_task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(task), std::forward<Args>(args)...)
        );
        
        std::future<return_type> future = packaged_task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            
            tasks_.emplace([packaged_task]() { (*packaged_task)(); });
        }
        
        condition_.notify_one();
        return future;
    }

    /**
     * @brief Submit multiple read tasks simultaneously
     * @param tasks Vector of read functions to execute
     * @return Vector of futures for the results
     */
    template<typename F>
    std::vector<std::future<std::invoke_result_t<F>>> submitReads(const std::vector<F>& tasks) {
        std::vector<std::future<std::invoke_result_t<F>>> futures;
        futures.reserve(tasks.size());
        
        for (const auto& task : tasks) {
            futures.push_back(submit(task));
        }
        
        return futures;
    }

    /**
     * @brief Submit multiple write tasks (note: caller must ensure proper synchronization)
     * @param tasks Vector of write functions to execute
     * @return Vector of futures for the results
     */
    template<typename F>
    std::vector<std::future<std::invoke_result_t<F>>> submitWrites(const std::vector<F>& tasks) {
        std::vector<std::future<std::invoke_result_t<F>>> futures;
        futures.reserve(tasks.size());
        
        for (const auto& task : tasks) {
            futures.push_back(submit(task));
        }
        
        return futures;
    }

    /**
     * @brief Wait for all currently submitted tasks to complete
     */
    void waitForTasks() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (tasks_.empty()) {
                break;
            }
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    /**
     * @brief Get the number of tasks currently queued
     * @return Number of queued tasks
     */
    size_t getTaskCount() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    /**
     * @brief Check if the thread pool is currently processing tasks
     * @return true if tasks are running
     */
    bool isRunning() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return !tasks_.empty();
    }
};

} // namespace scalerdb 