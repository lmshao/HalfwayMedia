//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "thread_pool.h"
#include "common/log.h"
#include <cstring>
#include <string>
#include <utility>

ThreadPool::ThreadPool(int preAlloc, int threadsMax, std::string name) : threadsMax_(threadsMax)
{
    if (preAlloc > threadsMax) {
        preAlloc = threadsMax;
    }

    if (!name.empty()) {
        if (name.size() > 12) {
            name = name.substr(0, 12);
        }
        threadName_ = name;
    }

    for (int i = 0; i < preAlloc; ++i) {
        auto p = std::make_unique<std::thread>(&ThreadPool::Worker, this);
        std::string threadName = threadName_ + "-" + std::to_string(i);
        pthread_setname_np(p->native_handle(), threadName.c_str());
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
    std::string tag;
    while (running_) {
        if (!tag.empty()) {
            if (tagTasks_.find(tag) != tagTasks_.end()) {
                if (!tagTasks_.at(tag).empty()) {
                    std::unique_lock<std::mutex> lock(taskMutex_);
                    auto task = tagTasks_.at(tag).front();
                    tagTasks_.at(tag).pop();
                    lock.unlock();
                    tag = task->tag;
                    if (task) {
                        auto fn = task->fn;
                        if (fn) {
                            fn((void *)task->data);
                        }
                    }
                    continue;
                } else {
                    tagTasks_.erase(tag);
                }
            }
        }

        tag.clear();
        while (tasks_.empty()) {
            std::unique_lock<std::mutex> taskLock(signalMutex_);
            idle_++;
            signal_.wait(taskLock);
            idle_--;
            if (!running_) {
                return;
            }
        }

        std::unique_lock<std::mutex> lock(taskMutex_);
        if (tasks_.empty()) {
            continue;
        }

        auto task = tasks_.front();
        tasks_.pop();
        lock.unlock();

        if (!task->tag.empty() && tagTasks_.find(task->tag) != tagTasks_.end()) {
            tagTasks_.at(task->tag).push(task);
            continue;
        }

        tag = task->tag;
        if (!tag.empty() && tagTasks_.find(tag) == tagTasks_.end()) {
            tagTasks_.emplace(tag, std::queue<std::shared_ptr<TaskItem>>{});
        }

        if (task) {
            auto fn = task->fn;
            if (fn) {
                fn((void *)task->data);
            }
        }
    }
}

void ThreadPool::AddTask(const Task &task)
{
    AddTask(task, nullptr, 0, "");
}

void ThreadPool::AddTask(const Task &task, const std::string &serialTag)
{
    AddTask(task, nullptr, 0, serialTag);
}

void ThreadPool::AddTask(const Task &task, void *userData, size_t dataSize)
{
    AddTask(task, userData, dataSize, "");
}

void ThreadPool::AddTask(const Task &task, void *userData, size_t dataSize, const std::string &serialTag)
{
    if (task == nullptr) {
        LOGE("task is nullptr");
        return;
    }

    auto t = std::make_shared<TaskItem>(task, userData, dataSize, serialTag);
    std::unique_lock<std::mutex> lock(taskMutex_);
    tasks_.push(t);
    lock.unlock();

    if (idle_ > 0) {
        signal_.notify_one();
        return;
    }

    LOGD("idle:%d, thread num: %zu/%d", idle_.load(), threads_.size(), threadsMax_);
    if (threads_.size() < threadsMax_) {
        auto p = std::make_unique<std::thread>(&ThreadPool::Worker, this);
        std::string threadName = threadName_ + "-" + std::to_string(threads_.size());
        pthread_setname_np(p->native_handle(), threadName_.c_str());
        threads_.emplace(std::move(p));
    }
}

ThreadPool::TaskItem::TaskItem(Task task, void *userData, size_t dataSize, std::string serialTag)
    : fn(std::move(task)), tag(std::move(serialTag))
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
