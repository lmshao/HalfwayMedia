//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_AAC_H
#define HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_AAC_H

#include "rtp_packet.h"

class RtpPacketizerAAC : public RtpPacketizer {
public:
    void Packetize(const std::shared_ptr<Frame> &frame) override;
};

class RtpDepacketizerAAC : public RtpDepacketizer {
public:
    void DepacketizeInner(std::shared_ptr<DataBuffer> dataBuffer) override;
    void SetExtraData(void *extra) override;

private:
    uint8_t channels_;
    uint32_t nbSamples_;
    uint32_t sampleRate_;
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_AAC_H