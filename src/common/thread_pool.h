//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_THREAD_POOL_H
#define HALFWAY_MEDIA_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

constexpr int THREAD_NUM_MAX = 20;
constexpr int THREAD_NUM_PRE_ALLOC = 4;

class ThreadPool {
public:
    using Task = std::function<void(void *)>;

    explicit ThreadPool(int preAlloc = THREAD_NUM_PRE_ALLOC, int threadsMax = THREAD_NUM_MAX, std::string name = "");
    ~ThreadPool();

    void Worker();

    void AddTask(const Task &task);
    void AddTask(const Task &task, const std::string &serialTag);
    void AddTask(const Task &task, void *userData, size_t dataSize);
    void AddTask(const Task &task, void *userData, size_t dataSize, const std::string &serialTag);

private:
    struct TaskItem {
        TaskItem(Task task, void *userData, size_t dataSize, std::string serialTag);
        ~TaskItem();

        Task fn;
        char *data = nullptr;
        size_t size = 0;
        std::string tag;
    };

private:
    bool running_ = true;
    int threadsMax_;
    std::atomic<int> idle_ = 0;
    std::string threadName_ = "threadpool";
    std::mutex signalMutex_;
    std::condition_variable signal_;

    std::mutex taskMutex_;
    std::queue<std::shared_ptr<TaskItem>> tasks_;
    std::unordered_map<std::string, std::queue<std::shared_ptr<TaskItem>>> tagTasks_;
    std::unordered_set<std::unique_ptr<std::thread>> threads_;
};

#endif // HALFWAY_MEDIA_THREAD_POOL_H