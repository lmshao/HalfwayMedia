//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_TCP_SERVER_H
#define HALFWAY_MEDIA_NETWORK_TCP_SERVER_H

#include "../common/thread_pool.h"
#include "base_server.h"
#include "iserver_listener.h"
#include "network_common.h"
#include "session_impl.h"
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>

class TcpServer final : public BaseServer, public std::enable_shared_from_this<TcpServer> {
    friend class EventProcessor;

public:
    template <typename... Args>
    static std::shared_ptr<TcpServer> Create(Args... args)
    {
        return std::shared_ptr<TcpServer>(new TcpServer(args...));
    }

    ~TcpServer();

    bool Init() override;
    void SetListener(std::shared_ptr<IServerListener> listener) override { listener_ = listener; }
    bool Start() override;
    bool Stop() override;
    bool Send(int fd, std::shared_ptr<DataBuffer> buffer) override;

protected:
    TcpServer(std::string listenIp, uint16_t listenPort) : localIp_(listenIp), localPort_(listenPort) {}
    explicit TcpServer(uint16_t listenPort) : localPort_(listenPort) {}

    void HandleAccept(int fd);
    void HandleReceive(int fd);

private:
    uint16_t localPort_;
    int socket_ = INVALID_SOCKET;
    std::string localIp_ = LOCAL_HOST;
    struct sockaddr_in serverAddr_;

    std::weak_ptr<IServerListener> listener_;
    std::unordered_map<int, std::shared_ptr<SessionImpl>> sessions_;
    std::unique_ptr<ThreadPool> callbackThreads_;
};

#endif // HALFWAY_MEDIA_NETWORK_TCP_SERVER_H