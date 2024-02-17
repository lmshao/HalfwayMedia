//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_packet.h"
#include "../../common/log.h"
#include "rtp_packet_aac.h"
#include "rtp_packet_h264.h"
#include <cstdint>

std::shared_ptr<RtpPacketizer> RtpPacketizer::Create(FrameFormat format)
{
    std::shared_ptr<RtpPacketizer> packetizer;
    switch (format) {
        case FRAME_FORMAT_H264:
            packetizer = std::make_shared<RtpPacketizerH264>();
            break;
        case FRAME_FORMAT_AAC:
            packetizer = std::make_shared<RtpPacketizerAAC>();
            break;
        default:
            LOGE("Unsupported frame format (%d).", format);
            break;
    }

    return packetizer;
}

void RtpPacketizer::FillRtpHeader(RtpHeader &header, uint8_t pt, uint32_t ts, bool mark)
{
    header.SetSSRC(GetSSRC());
    header.SetSeqNumber(seqNumber_++);
    header.SetPayloadType(pt);
    header.SetTimestamp(ts);
    header.SetMarker(mark);
}

std::shared_ptr<RtpDepacketizer> RtpDepacketizer::Create(FrameFormat format)
{
    std::shared_ptr<RtpDepacketizer> depacketizer;
    switch (format) {
        case FRAME_FORMAT_H264:
            depacketizer = std::make_shared<RtpDepacketizerH264>();
            break;
        case FRAME_FORMAT_AAC:
            depacketizer = std::make_shared<RtpDepacketizerAAC>();
            break;
        default:
            LOGE("Unsupported frame format (%d).", format);
            break;
    }

    return depacketizer;
}

RtpDepacketizer::RtpDepacketizer()
{
    sorter_.SetCallback([this](std::shared_ptr<DataBuffer> packet) { DepacketizeInner(packet); });
}