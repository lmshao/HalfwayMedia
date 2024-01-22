//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_NETWORK_COMMON_H
#define HALFWAY_MEDIA_NETWORK_NETWORK_COMMON_H

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

const int INVALID_SOCKET = -1;
const std::string LOCAL_HOST = "0.0.0.0";

const int TCP_BACKLOG = 10;

struct ClientInfo {
    int fd;
    std::string host;
    uint16_t port;

    std::string ToString() const
    {
        std::stringstream ss;
        ss << host << ":" << port << " (" << fd << ")";
        return ss.str();
    }
};

class InternalFdListener {
public:
    virtual ~InternalFdListener() = default;
    virtual void OnAccept(const ClientInfo &client) = 0;
    virtual void OnReceive(const ClientInfo &client) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_NETWORK_COMMON_H