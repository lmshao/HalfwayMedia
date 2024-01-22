//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "event_processor.h"
#include "../common/utils.h"
#include "event_reactor.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>

constexpr int EPOLL_WAIT_EVENT_NUMS_MAX = 1024;
constexpr int REACTOR_NUMS_MAX = 1;

EventProcessor::EventProcessor() {}

EventProcessor::~EventProcessor()
{
    running_ = false;
}

void EventProcessor::Start()
{
    LOGD("enter");
}

void EventProcessor::HandleAccept(int fd)
{
    LOGD("...");

    struct sockaddr_in clientAddr = {};
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int clientSocket = accept(fd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSocket < 0) {
        LOGE("accept error: %s", strerror(errno));
        return;
    }

    LOGD("...");

    std::string host = inet_ntoa(clientAddr.sin_addr);
    uint16_t port = ntohs(clientAddr.sin_port);

    LOGD("New client connections client[%d] %s:%d\n", clientSocket, inet_ntoa(clientAddr.sin_addr),
         ntohs(clientAddr.sin_port));

    auto listener = GetListener(fd);
    if (listener) {
        listener->OnAccept(ClientInfo{clientSocket, host, port});
    }
    LOGD("fd: %d, listener: %p", fd, listener.get());

    // 查找当前server fd 对应的subReactor

    auto it = std::find_if(reactorFdsMap_.begin(), reactorFdsMap_.end(), [fd](auto &item) {
        std::unordered_set<int> &fdSet = item.second;
        return fdSet.find(fd) != fdSet.end();
    });

    LOGD("...");

    std::shared_ptr<EventReactor> subReactor;
    if (it != reactorFdsMap_.end()) {
        subReactor = it->first;
    } else {
        LOGD("reactorFdsMap_ size: %zu", reactorFdsMap_.size());
        if (reactorFdsMap_.size() >= REACTOR_NUMS_MAX) {
            LOGD("...");

            // 随机获取一个subReactor
            subReactor = reactorFdsMap_.begin()->first;
        } else {
            // 新建一个
            LOGD("...");

            subReactor = std::make_shared<EventReactor>();
            std::unordered_set<int> fdSet;
            fdSet.insert(fd);
            reactorFdsMap_.emplace(subReactor, fdSet);
        }
    }

    subReactor->AddListeningFd(clientSocket, [&, fd, host, port](int cfd) {
        auto listener = GetListener(fd);
        if (listener) {
            listener->OnReceive(ClientInfo{cfd, host, port});
        }
    });
}

void EventProcessor::AddListeningFd(int fd, std::shared_ptr<InternalFdListener> listener)
{
    std::call_once(initFlag_, [&]() { this->Start(); });

    LOGD("fd: %d, listener: %p", fd, listener.get());
    std::unique_lock<std::mutex> lock(mutex_);
    listeners_.emplace(fd, listener);
    lock.unlock();

    if (mainReactor_ == nullptr) {
        mainReactor_ = std::make_unique<EventReactor>();
        mainReactor_->SetThreadName("MainReactor");

        for (int i = 0; i < REACTOR_NUMS_MAX; i++) {
            auto subReactor = std::make_shared<EventReactor>();
            std::unordered_set<int> s;
            subReactor->SetThreadName("SubReactor-" + std::to_string(i));
            reactorFdsMap_.emplace(subReactor, s);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    LOGD("... fd:%d", fd);
    mainReactor_->AddListeningFd(fd, [&](int fd) {
        this->HandleAccept(fd);
        LOGD("HandleAccept over");
    });
    LOGD("...");
}

void EventProcessor::RemoveListeningFd(int fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.erase(fd);
    // TODO: find connection fd, remove all
    // TODO: find reactor,remove from list
}

void EventProcessor::RemoveConnectionFd(int fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // TODO: find reactor, remove from listening list
}

std::shared_ptr<InternalFdListener> EventProcessor::GetListener(int fd)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = listeners_.find(fd);
    if (it == listeners_.end()) {
        LOGE("Can not find listener of fd (%d)", fd);
        return nullptr;
    }

    if (it->second.expired()) {
        LOGE("listener of fd (%d) is expired", fd);
        return nullptr;
    }

    return it->second.lock();
}
