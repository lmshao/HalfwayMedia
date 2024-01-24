//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_SESSION_H
#define HALFWAY_MEDIA_NETWORK_SESSION_H

#include "../common/data_buffer.h"
#include "../common/log.h"
#include "base_server.h"
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

class Session {
public:
    std::string host;
    uint16_t port;

    virtual bool Send(std::shared_ptr<DataBuffer> buffer) const = 0;
    virtual std::string ClientInfo() const = 0;

protected:
    Session() = default;
    virtual ~Session() = default;
};

#endif // HALFWAY_MEDIA_NETWORK_SESSION_H