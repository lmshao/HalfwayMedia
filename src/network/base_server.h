//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_BASE_SERVER_H
#define HALFWAY_MEDIA_NETWORK_BASE_SERVER_H

#include "../common/data_buffer.h"
#include <memory>

class IServerListener;
class BaseServer {
public:
    virtual ~BaseServer() = default;
    virtual bool Init() = 0;
    virtual void SetListener(std::shared_ptr<IServerListener> listener) = 0;
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool Send(int fd, std::shared_ptr<DataBuffer> buffer) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_BASE_SERVER_H