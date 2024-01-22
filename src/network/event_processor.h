//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_EVENT_PROCESSOR_H
#define HALFWAY_MEDIA_NETWORK_EVENT_PROCESSOR_H

#include "../common/singleton.h"
#include "../network/network_common.h"
#include "event_reactor.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class EventProcessor : public Singleton<EventProcessor> {
    friend class Singleton<EventProcessor>;

public:
    ~EventProcessor() override;
    void AddListeningFd(int fd, std::shared_ptr<InternalFdListener> listener);
    void RemoveListeningFd(int fd);
    void RemoveConnectionFd(int fd);

protected:
    EventProcessor();

private:
    void Start();
    void HandleAccept(int fd);

    std::shared_ptr<InternalFdListener> GetListener(int fd);

private:
    std::once_flag initFlag_;
    bool running_ = true;

    std::mutex mutex_;
    // for TCP server socket acceptor
    std::unique_ptr<EventReactor> mainReactor_; // for accept

    std::unordered_map<std::shared_ptr<EventReactor>, std::unordered_set<int>> reactorFdsMap_;
    std::unordered_map<int, std::unordered_set<int>> fds_;                 // server socket fd - clients's socket fd
    std::unordered_map<int, std::weak_ptr<InternalFdListener>> listeners_; // server socket fd - listener
};

#endif // HALFWAY_MEDIA_NETWORK_EVENT_PROCESSOR_H