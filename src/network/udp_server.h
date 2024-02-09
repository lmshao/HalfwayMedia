//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_UDP_SERVER_H
#define HALFWAY_MEDIA_NETWORK_UDP_SERVER_H

#include "../common/thread_pool.h"
#include "base_server.h"
#include "iserver_listener.h"
#include "session_impl.h"
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <unordered_map>

class UdpServer final : public BaseServer, public std::enable_shared_from_this<UdpServer> {
    friend class EventProcessor;
    const int INVALID_SOCKET = -1;

public:
    template <typename... Args>
    static std::shared_ptr<UdpServer> Create(Args... args)
    {
        return std::shared_ptr<UdpServer>(new UdpServer(args...));
    }

    ~UdpServer();

    bool Init() override;
    void SetListener(std::shared_ptr<IServerListener> listener) override { listener_ = listener; }
    bool Start() override;
    bool Stop() override;
    bool Send(int fd, std::string host, uint16_t port, std::shared_ptr<DataBuffer> buffer) override;

    static uint16_t GetIdlePort();
    static uint16_t GetIdlePortPair();

protected:
    UdpServer(std::string listenIp, uint16_t listenPort) : localIp_(listenIp), localPort_(listenPort) {}
    explicit UdpServer(uint16_t listenPort) : localPort_(listenPort) {}

    void HandleReceive(int fd);

private:
    std::string localIp_ = "0.0.0.0";
    uint16_t localPort_;

    int socket_ = INVALID_SOCKET;
    struct sockaddr_in serverAddr_;

    std::weak_ptr<IServerListener> listener_;
    std::unordered_map<int, std::shared_ptr<SessionImpl>> sessions_;
    std::unique_ptr<ThreadPool> callbackThreads_;
};

#endif // HALFWAY_MEDIA_NETWORK_UDP_SERVER_H