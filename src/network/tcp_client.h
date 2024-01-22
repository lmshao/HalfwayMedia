//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H
#define HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H

#include "../common/data_buffer.h"
#include "network_common.h"
#include <cstdint>
#include <string>

class TcpClient {
public:
    TcpClient(std::string remoteIp, uint16_t remotePort, std::string localIp = "", uint16_t localPort = 0) {}

    bool Init() { return true; }

    bool Send(const std::string &str);
    bool Send(const char *data, size_t len);
    bool Send(std::shared_ptr<DataBuffer> data);

    bool Close();


private:
    
};

#endif // HALFWAY_MEDIA_NETWORK_TCP_CLIENT_H