//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H
#define HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H

#include "../common/data_buffer.h"
#include <memory>

class IClientListener {
public:
    virtual ~IClientListener() = default;
    virtual void OnReceive(std::shared_ptr<DataBuffer> buffer) = 0;
    virtual void OnClose() = 0;
    virtual void OnError(const std::string &errorInfo) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H