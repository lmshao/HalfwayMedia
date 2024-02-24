//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_pusher_session.h"
#include "../agent/media_file/media_file_source.h"
#include "../agent/rtp_stream/rtp_sink.h"
#include "../common/log.h"
#include "../common/utils.h"
#include "agent/base/event_definition.h"

bool RtpPusherSession::Init()
{
    if (url_.empty()) {
        LOGE("url is empty");
        return false;
    }

    if (rtpIp_.empty() || (videoPort_ == 0 && audioPort_ == 0)) {
        LOGE("rtp address is invalid %s:%d", rtpIp_.c_str(), videoPort_);
        return false;
    }

    UrlType type = DetectUrlType(url_);

    switch (type) {
        case TYPE_FILE:
            source_ = MediaFileSource::Create(url_);
            break;
        default:
            LOGE("Unknown URL type (%s)", url_.c_str());
            break;
    }

    if (!source_->Init()) {
        LOGE("source init failed");
        return false;
    }

    sink_ = RtpSink::Create(rtpIp_, videoPort_, audioPort_);

    if (!sink_->Notify(AgentEvent{EVENT_SINK_INIT})) {
        LOGE("source init failed");
        return false;
    }

    if (audioPort_ > 0) {
        source_->AddVideoSink(sink_);
    }

    if (videoPort_ > 0) {
        source_->AddAudioSink(sink_);
    }

    return true;
}

bool RtpPusherSession::Start()
{
    if (!source_->Start()) {
        LOGD("source started failed");
        return false;
    }

    LOGD("source started");
    return true;
}
