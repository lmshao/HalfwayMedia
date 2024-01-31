//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_sink.h"
#include "../../common/log.h"
#include <memory>

RtpSink::~RtpSink() {}

bool RtpSink::Init()
{
    if (remoteVideoPort_ == 0 && remoteAudioPort_ == 0) {
        LOGE("invalid remote video & audio port");
        return false;
    }

    if (remoteVideoPort_ > 0) {
        videoUdpClient_ = std::make_unique<UdpClient>(remoteIp_, remoteVideoPort_, "", localVideoPort_);
        if (!videoUdpClient_ || !videoUdpClient_->Init()) {
            LOGE("videoUdpClient init failed, remote %s:%d, local: ::%d", remoteIp_.c_str(), remoteVideoPort_,
                 localVideoPort_);
            return false;
        }
    }

    if (remoteAudioPort_ > 0) {
        audioUdpClient_ = std::make_unique<UdpClient>(remoteIp_, remoteAudioPort_, "", localAudioPort_);
        if (!audioUdpClient_ || !audioUdpClient_->Init()) {
            LOGE("audioUdpClient init failed, remote %s:%d, local: ::%d", remoteIp_.c_str(), remoteAudioPort_,
                 localAudioPort_);
            return false;
        }
    }

    LOGD("RtpSink Init ok");
    return true;
}

void RtpSink::OnFrame(const std::shared_ptr<Frame> &frame)
{
    if (frame->format == FRAME_FORMAT_H264) {
        if (!videoPacketizer_) {
            videoPacketizer_ = RtpPacketizer::Create(frame->format);
            if (!videoPacketizer_) {
                return;
            }

            videoPacketizer_->SetCallback([&](std::shared_ptr<DataBuffer> packet) {
                if (videoUdpClient_->Send(packet)) {
                    LOGD("send video packet(size: %zu)", packet->Size());
                }
            });
        }

        videoPacketizer_->Packetize(frame);
    } else if (frame->format == FRAME_FORMAT_AAC) {
        if (!audioPacketizer_) {
            audioPacketizer_ = RtpPacketizer::Create(frame->format);
            if (!audioPacketizer_) {
                return;
            }

            audioPacketizer_->SetCallback([&](std::shared_ptr<DataBuffer> packet) {
                if (audioUdpClient_->Send(packet)) {
                    LOGD("send audio packet(size: %zu)", packet->Size());
                }
            });
        }

        audioPacketizer_->Packetize(frame);
    }
}