//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtsp_client_session.h"
#include "../agent/media_file/raw_file_sink.h"
#include "../agent/rtsp_stream/rtsp_source.h"
#include "../common/log.h"
#include "../common/utils.h"

bool RtspClientSession::Init()
{
    if (url_.empty()) {
        LOGE("url is empty");
        return false;
    }

    if (url_.empty() || filename_.empty()) {
        LOGE("url(%s) or filename(%s) is invalid", url_.c_str(), filename_.c_str());
        return false;
    }

    UrlType type = DetectUrlType(url_);
    if (type != TYPE_RTSP) {
        LOGE("Unknown URL type (%s)", url_.c_str());
        return false;
    }

    source_ = RtspSource::Create(url_);

    if (!source_->Init()) {
        LOGE("source init failed");
        return false;
    }

    sink_ = RawFileSink::Create(filename_ + ".h264");

    if (!sink_->Init()) {
        LOGE("source init failed");
        return false;
    }

    source_->AddVideoSink(sink_);
    // source_->AddAudioSink(sink_);
    return true;
}

bool RtspClientSession::Start()
{
    if (!source_->Start()) {
        LOGD("source started failed");
        return false;
    }

    LOGD("source started");
    return true;
}