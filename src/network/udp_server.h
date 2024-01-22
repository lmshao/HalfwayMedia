//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_UDP_SERVER_H
#define HALFWAY_MEDIA_NETWORK_UDP_SERVER_H

#include <cstdint>
#include <string>

class UdpServer {
public:
    UdpServer(std::string listenIp, uint16_t listenPort);
    UdpServer(uint16_t listenPort);

    bool Start();
};

#endif // HALFWAY_MEDIA_NETWORK_UDP_SERVER_H