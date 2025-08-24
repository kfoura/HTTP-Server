#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>


class ThreadPool {
    public:
        ThreadPool(size_t numThreads);
        ~ThreadPool();

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    private:
        std::vector<std::thread> workers{};
        std::queue<std::function<void()>> tasks{};

        std::mutex queueMutex{};
        std::condition_variable condition{};
        std::atomic<bool> stop{false};

        void workerThread();
};

// ------------------ Implementation ------------------
inline ThreadPool::ThreadPool(size_t numThreads) : stop (false){
    for (size_t i = 0; i < numThreads; ++i){
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

inline ThreadPool::~ThreadPool(){
    stop = true;
    condition.notify_all();
    for (auto &worker : workers){
        worker.join();
    }
}

inline void ThreadPool::workerThread() {
    while (!stop) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]{ return stop || !tasks.empty(); });
            if (stop && tasks.empty()){
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock (queueMutex);
        tasks.emplace([task]() {(*task)();});
    }
    condition.notify_one();
    return res;
}