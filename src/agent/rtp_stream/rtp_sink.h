//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_RTP_SINK_H
#define HALFWAY_MEDIA_RTP_SINK_H

#include "../../network/udp_client.h"
#include "../../protocol/rtp/rtp_packet.h"
#include "../base/media_sink.h"
#include <cstdint>
#include <memory>

class RtpSink : public MediaSink {
public:
    ~RtpSink() override;

    static std::shared_ptr<RtpSink> Create(std::string remoteIp, uint16_t remoteVideoPort, uint16_t remoteAudioPort = 0)
    {
        return std::shared_ptr<RtpSink>(new RtpSink(std::move(remoteIp), remoteVideoPort, remoteAudioPort));
    }

    // impl MediaSink
    bool Init() override;

    // impl FrameSink
    void OnFrame(const std::shared_ptr<Frame> &frame) override;

    void SetLocalPort(uint16_t videPort, uint16_t audioPort = 0)
    {
        localVideoPort_ = videPort;
        localAudioPort_ = audioPort;
    }

private:
    explicit RtpSink(std::string remoteIp, uint16_t remoteVideoPort, uint16_t remoteAudioPort = 0)
        : remoteIp_(remoteIp), remoteVideoPort_(remoteVideoPort), remoteAudioPort_(remoteAudioPort)
    {
    }

private:
    std::string remoteIp_;
    uint16_t remoteVideoPort_ = 0;
    uint16_t remoteAudioPort_ = 0;
    uint16_t localVideoPort_ = 0;
    uint16_t localAudioPort_ = 0;

    std::unique_ptr<UdpClient> videoUdpClient_;
    std::unique_ptr<UdpClient> audioUdpClient_;

    std::shared_ptr<RtpPacketizer> videoPacketizer_;
    std::shared_ptr<RtpPacketizer> audioPacketizer_;
};

#endif // HALFWAY_MEDIA_RTP_SINK_H