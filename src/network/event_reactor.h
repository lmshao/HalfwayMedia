//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_EVENT_REACTOR_H
#define HALFWAY_MEDIA_NETWORK_EVENT_REACTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

class EventReactor {
public:
    EventReactor();
    ~EventReactor();

    void AddListeningFd(int fd, std::function<void(int)> callback);
    void SetThreadName(std::string name);

private:
    void Run();

private:
    int epollFd_ = -1;
    std::mutex mutex_;

    bool running_ = true;

    std::mutex signalMutex_;
    std::condition_variable runningSignal_;

    std::unordered_map<int, std::function<void(int)>> fds_;
    std::unique_ptr<std::thread> epollThread_;
};
#endif // HALFWAY_MEDIA_NETWORK_EVENT_REACTOR_H