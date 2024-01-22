//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H
#define HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H

#include "../common/data_buffer.h"
#include <memory>

class IClientListener {
public:
    IClientListener();
    virtual void OnReceive(std::shared_ptr<DataBuffer> buffer) = 0;
};

#endif // HALFWAY_MEDIA_NETWORK_ICLIENT_LISTENER_H