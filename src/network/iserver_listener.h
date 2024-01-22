//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H
#define HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H

#include "../common/data_buffer.h"
#include "network_common.h"
#include <memory>

class IServerListener {
public:
    virtual ~IServerListener() = default;
    virtual void OnError(const ClientInfo &client) = 0;
    virtual void OnClose(const ClientInfo &client) = 0;
    virtual void OnAccept(const ClientInfo &client) = 0;
    virtual void OnReceive(const ClientInfo &client, std::shared_ptr<DataBuffer> buffer) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H