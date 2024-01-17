//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "thread_pool.h"
#include <cstring>

ThreadPool::ThreadPool(int preAlloc, int threadsMax) : threadsMax_(threadsMax)
{
    if (preAlloc > threadsMax) {
        preAlloc = threadsMax;
    }
    for (int i = 0; i < preAlloc; ++i) {
        auto p = std::make_unique<std::thread>(&ThreadPool::Worker, this);
        threads_.emplace(std::move(p));
    }
}

ThreadPool::~ThreadPool()
{
    running_ = false;
    signal_.notify_all();
    for (auto &thread : threads_) {
        if (thread->joinable()) {
            thread->join();
        }
    }
}

void ThreadPool::Worker()
{
    while (running_) {
        if (tasks_.empty()) {
            std::unique_lock<std::mutex> taskLock(signalMutex_);
            idle_++;
            signal_.wait(taskLock);
            idle_--;
            if (!running_) {
                return;
            }
        }

        if (tasks_.empty()) {
            continue;
        }

        std::unique_lock<std::mutex> lock(taskMutex_);
        auto task = tasks_.front();
        tasks_.pop();
        lock.unlock();

        if (task && task->fn) {
            task->fn((void *)task->data);
        }
    }
}

void ThreadPool::AddTask(const Task &task, void *userData, size_t dataSize)
{
    if (task == nullptr) {
        return;
    }

    auto t = std::make_shared<TaskItem>(task, userData, dataSize);
    tasks_.emplace(t);

    if (idle_ > 0) {
        signal_.notify_one();
        return;
    }

    if (threads_.size() < threadsMax_) {
        auto p = std::make_unique<std::thread>(&ThreadPool::Worker, this);
        threads_.emplace(std::move(p));
    }
}

ThreadPool::TaskItem::TaskItem(Task task, void *userData, size_t dataSize) : fn(std::move(task))
{
    if (userData) {
        size = dataSize;
        data = new char[size];
        memcpy(data, userData, size);
    }
}

ThreadPool::TaskItem::~TaskItem()
{
    delete[] data;
}
