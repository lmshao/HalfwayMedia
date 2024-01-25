//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_SESSION_IMPL_H
#define HALFWAY_MEDIA_NETWORK_SESSION_IMPL_H

#include "session.h"

class SessionImpl : public Session {
public:
    SessionImpl(int fd, std::string host, uint16_t port, std::shared_ptr<BaseServer> server) : fd_(fd), server_(server)
    {
        this->host = host;
        this->port = port;
    }

    virtual ~SessionImpl() = default;

    bool Send(std::shared_ptr<DataBuffer> buffer) const
    {
        auto server = server_.lock();
        if (server) {
            return server->Send(fd_, host, port, buffer);
        }
        LOGE("server is invalid");
        return false;
    }

    std::string ClientInfo() const
    {
        std::stringstream ss;
        ss << host << ":" << port << " (" << fd_ << ")";
        return ss.str();
    }

private:
    int fd_ = 0;
    std::weak_ptr<BaseServer> server_;
};

#endif // HALFWAY_MEDIA_NETWORK_SESSION_IMPL_H