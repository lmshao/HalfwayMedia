//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "event_reactor.h"
#include "../common/log.h"
#include <cstring>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <thread>

constexpr int EPOLL_WAIT_EVENT_NUMS_MAX = 1024;

EventReactor::EventReactor()
{
    epollThread_ = std::make_unique<std::thread>([&]() { this->Run(); });
    std::unique_lock<std::mutex> taskLock(signalMutex_);
    runningSignal_.wait_for(taskLock, std::chrono::milliseconds(5), [this] { return this->running_ == true; });
}

EventReactor::~EventReactor()
{
    LOGD("enter");
    running_ = false;
    if (epollThread_ && epollThread_->joinable()) {
        epollThread_->join();
    }
}

void EventReactor::AddListeningFd(int fd, std::function<void(int)> callback)
{
    LOGD("[%p] ... fd:%d", this, fd);
    std::unique_lock<std::mutex> lock(mutex_);
    fds_.emplace(fd, callback);
    lock.unlock();

    if (!running_) {
        LOGE("Reactor has exited");
        return;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        LOGE("epoll_ctl error: %s", strerror(errno));
        return;
    }
    // LOGD("epoll_ctl ok");
}

void EventReactor::RemoveListeningFd(int fd)
{
    LOGD("remove fd(%d)", fd);
    std::unique_lock<std::mutex> lock(mutex_);
    if (fds_.find(fd) == fds_.end()) {
        return;
    }
    fds_.erase(fd);
    lock.unlock();

    if (running_ == false) {
        LOGE("Reactor has exited");
        return;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev) == -1) {
        LOGE("epoll_ctl error: %s", strerror(errno));
        return;
    }
}

void EventReactor::Run()
{
    LOGD("enter");
    epollFd_ = epoll_create1(0);
    if (epollFd_ == -1) {
        perror("epoll_create");
        LOGE("epoll_create %s", strerror(errno));
        return;
    }

    int nfds = 0;
    running_ = true;
    runningSignal_.notify_all();
    struct epoll_event readyEvents[EPOLL_WAIT_EVENT_NUMS_MAX] = {};

    while (running_) {
        nfds = epoll_wait(epollFd_, readyEvents, EPOLL_WAIT_EVENT_NUMS_MAX, 100);
        if (nfds == -1) {
            LOGE("epoll_wait error: %s", strerror(errno));
            return;
        } else if (nfds == 0) {
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int readyFd = readyEvents[i].data.fd;
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = fds_.find(readyFd);
            if (it != fds_.end()) {
                auto callback = it->second;
                lock.unlock();

                callback(readyFd);
            }
        }
    }
}

void EventReactor::SetThreadName(std::string name)
{
    if (epollThread_ && !name.empty()) {
        if (name.size() > 15) {
            name = name.substr(0, 15);
        }
        pthread_setname_np(epollThread_->native_handle(), name.c_str());
    }
}
