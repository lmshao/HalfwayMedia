//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_source.h"

bool MediaSource::Init()
{
    return true;
}

bool MediaSource::Start()
{
    workerThread_ = std::make_unique<std::thread>(&MediaSource::ReceiveDataLoop, this);
    return true;
}

MediaSource::~MediaSource()
{
    running_ = false;
    if (workerThread_->joinable()) {
        workerThread_->join();
    }
    workerThread_.reset();
}
