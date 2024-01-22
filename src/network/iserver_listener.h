//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H
#define HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H

#include "../common/data_buffer.h"
#include "session.h"
#include <memory>

class IServerListener {
public:
    virtual ~IServerListener() = default;
    virtual void OnError(std::shared_ptr<Session> session, const std::string &errorInfo) = 0;
    virtual void OnClose(std::shared_ptr<Session> session) = 0;
    virtual void OnAccept(std::shared_ptr<Session> session) = 0;
    virtual void OnReceive(std::shared_ptr<Session> session, std::shared_ptr<DataBuffer> buffer) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_ISERVER_LISTENER_H