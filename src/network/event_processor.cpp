//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "event_processor.h"
#include "../common/log.h"
#include "event_reactor.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_set>

constexpr int EPOLL_WAIT_EVENT_NUMS_MAX = 1024;
constexpr int REACTOR_NUMS_MAX = 2;

EventProcessor::EventProcessor() {}

EventProcessor::~EventProcessor() {}

void EventProcessor::AddServiceFd(int fd, std::function<void(int)> callback)
{
    LOGD("fd: %d", fd);
    std::unique_lock<std::mutex> lock(callbackMutex_);
    fdCallbacks_.emplace(fd, callback);
    lock.unlock();

    if (mainReactor_ == nullptr) {
        mainReactor_ = std::make_unique<EventReactor>();
        mainReactor_->SetThreadName("MainReactor");
    }

    mainReactor_->AddListeningFd(fd, callback);
    LOGD("leave");
}

void EventProcessor::RemoveServiceFd(int fd)
{
    std::unique_lock<std::mutex> lock(callbackMutex_);
    fdCallbacks_.erase(fd);
    lock.unlock();
    if (mainReactor_) {
        mainReactor_->RemoveListeningFd(fd);
    }
}

void EventProcessor::AddConnectionFd(int fd, std::function<void(int)> callback)
{
    std::unique_lock<std::mutex> lock(callbackMutex_);
    fdCallbacks_.emplace(fd, callback);
    lock.unlock();

    std::unique_lock<std::mutex> reactorLock(subReactorsMutex_);
    std::shared_ptr<EventReactor> subReactor;
    if (subReactorsMap_.size() < REACTOR_NUMS_MAX) {
        subReactor = std::make_shared<EventReactor>();
        subReactor->SetThreadName("SubReactor-" + std::to_string(subReactorsMap_.size()));
        subReactorsMap_.emplace(subReactor, std::unordered_set<int>{fd});
    } else {
        // find the most idle reactor
        std::vector<std::pair<size_t, std::shared_ptr<EventReactor>>> vec;
        for (auto &it : subReactorsMap_) {
            vec.emplace_back(it.second.size(), it.first);
        }
        std::sort(vec.begin(), vec.end(), [](auto &a, auto &b) { return a.first < b.first; });
        subReactor = vec.begin()->second;

        subReactorsMap_[subReactor].emplace(fd);
    }
    reactorLock.unlock();
    subReactor->AddListeningFd(fd, callback);
}

void EventProcessor::RemoveConnectionFd(int fd)
{
    std::unique_lock<std::mutex> lock(callbackMutex_);
    if (fdCallbacks_.find(fd) == fdCallbacks_.end()) {
        return;
    }
    fdCallbacks_.erase(fd);
    lock.unlock();

    std::unique_lock<std::mutex> reactorLock(subReactorsMutex_);
    for (auto &item : subReactorsMap_) {
        if (item.second.find(fd) != item.second.end()) {
            auto subReactor = item.first;
            subReactor->RemoveListeningFd(fd);
            item.second.erase(fd);
            break;
        }
    }
}
