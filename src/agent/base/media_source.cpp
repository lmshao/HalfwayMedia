//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_source.h"
#include "common/log.h"
#include "event_definition.h"

bool MediaSource::Init()
{
    return true;
}

bool MediaSource::Start()
{
    workerThread_ = std::make_unique<std::thread>(&MediaSource::ReceiveDataLoop, this);
    return workerThread_ != nullptr;
}

MediaSource::~MediaSource()
{
    running_ = false;
    if (workerThread_->joinable()) {
        workerThread_->join();
    }
    workerThread_.reset();
}

bool MediaSource::NotifySink(AgentEvent event)
{
    return FrameSource::NotifySink(&event);
}

void MediaSource::OnNotify(void *userdata)
{
    AgentEvent *event = (AgentEvent *)userdata;

    switch (event->type) {
        case EVENT_SOURCE_ERROR:
            if (event->info) {
                LOGE("OnError: %s", event->info->info.c_str());
            }
            break;
        default:
            LOGW("unsupported event : %d", (int)event->type);
            break;
    }
}