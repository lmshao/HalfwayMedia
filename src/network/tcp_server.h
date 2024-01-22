//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_TCP_SERVER_H
#define HALFWAY_MEDIA_NETWORK_TCP_SERVER_H

#include "../common/thread_pool.h"
#include "iserver_listener.h"
#include "network_common.h"
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>

class TcpServer final : public InternalFdListener, public std::enable_shared_from_this<TcpServer> {
    friend class EventProcessor;

public:
    template <typename... Args>
    static std::shared_ptr<TcpServer> Create(Args... args)
    {
        return std::shared_ptr<TcpServer>(new TcpServer(args...));
    }

    ~TcpServer() override{  }

    bool Init();
    void SetListener(std::shared_ptr<IServerListener> listener) { listener_ = listener; }
    bool Start();
    bool Stop();

protected:
    TcpServer(std::string listenIp, uint16_t listenPort) : localIp_(listenIp), localPort_(listenPort) {}
    explicit TcpServer(uint16_t listenPort) : localPort_(listenPort) {}

    void OnAccept(const ClientInfo &client) override;
    void OnReceive(const ClientInfo &client) override;

private:
    uint16_t localPort_;
    int socket_ = INVALID_SOCKET;
    std::string localIp_ = LOCAL_HOST;
    struct sockaddr_in serverAddr_;

    std::weak_ptr<IServerListener> listener_;
    std::unique_ptr<ThreadPool> callbackThreads_;
};

#endif // HALFWAY_MEDIA_NETWORK_TCP_SERVER_H