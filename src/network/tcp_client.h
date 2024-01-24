//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H
#define HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H

#include "../common/data_buffer.h"
#include "../common/thread_pool.h"
#include "iclient_listener.h"
#include "network_common.h"
#include <cstdint>
#include <netinet/in.h>
#include <string>

class TcpClient final {
    friend class EventProcessor;

public:
    template <typename... Args>
    static std::shared_ptr<TcpClient> Create(Args... args)
    {
        return std::shared_ptr<TcpClient>(new TcpClient(args...));
    }

    ~TcpClient();

    bool Init();
    void SetListener(std::shared_ptr<IClientListener> listener) { listener_ = listener; }
    bool Connect();

    bool Send(const std::string &str);
    bool Send(const char *data, size_t len);
    bool Send(std::shared_ptr<DataBuffer> data);

    bool Close();

protected:
    TcpClient(std::string remoteIp, uint16_t remotePort, std::string localIp = "", uint16_t localPort = 0);

    void HandleReceive(int fd);

private:
    std::string remoteIp_;
    uint16_t remotePort_;

    std::string localIp_;
    uint16_t localPort_;

    int socket_ = INVALID_SOCKET;
    struct sockaddr_in serverAddr_;

    std::weak_ptr<IClientListener> listener_;
    std::unique_ptr<ThreadPool> callbackThreads_;
};

#endif // HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H