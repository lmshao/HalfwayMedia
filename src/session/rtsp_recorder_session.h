//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_SESSION_RTSP_RECORDER_SESSION_H
#define HALFWAY_MEDIA_SESSION_RTSP_RECORDER_SESSION_H

#include "../agent/base/media_sink.h"
#include "../agent/base/media_source.h"
#include <string>

class RtspRecoderSession {
public:
    RtspRecoderSession() = default;
    void SetSourceUrl(std::string url) { url_ = url; }
    void SetRecorderFileName(std::string filename) { filename_ = filename; }

    bool Init();

    bool Start();

public:
    std::string url_;
    std::string filename_;

    std::shared_ptr<MediaSource> source_;
    std::shared_ptr<MediaSink> sink_;
};

#endif // HALFWAY_MEDIA_SESSION_RTSP_RECORDER_SESSION_H