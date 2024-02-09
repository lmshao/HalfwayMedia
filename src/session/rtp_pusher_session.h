//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_SESSION_RTP_PUSHER_SESSION_H
#define HALFWAY_MEDIA_SESSION_RTP_PUSHER_SESSION_H

#include "../agent/base/media_sink.h"
#include "../agent/base/media_source.h"
#include <cstdint>
#include <string>

class RtpPusherSession {
public:
    RtpPusherSession() = default;
    void SetSourceUrl(std::string url) { url_ = url; }
    void SetRtpAddress(std::string remoteIp, uint16_t videoPort, uint16_t audioPort)
    {
        rtpIp_ = remoteIp;
        videoPort_ = videoPort;
        audioPort_ = audioPort;
    }

    bool Init();

    bool Start();

public:
    std::string url_;
    std::string rtpIp_;
    uint16_t videoPort_;
    uint16_t audioPort_;

    std::shared_ptr<MediaSource> source_;
    std::shared_ptr<MediaSink> sink_;
};

#endif // HALFWAY_MEDIA_SESSION_RTP_PUSHER_SESSION_H