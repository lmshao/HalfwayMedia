//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_packet_aac.h"
#include "../../common/log.h"
#include "../aac/adts_header.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <netinet/in.h>

void RtpPacketizerAAC::Packetize(const std::shared_ptr<Frame> &frame)
{
    LOGD("enter");

    LOGD("samplerate: %d, channels: %d,nbSample: %d, ts: %lu ", frame->audioInfo.sampleRate, frame->audioInfo.channels,
         frame->audioInfo.nbSamples, frame->timestamp);

    const uint8_t *p = frame->Data();
    size_t size = frame->Size();

    if (0xFF == p[0] && 0xF0 == (p[1] & 0xF0) && size > 7) {
        LOGD("skip ADTS header");
        if (size != (((p[3] & 0x03) << 11) | (p[4] << 3) | ((p[5] >> 5) & 0x07))) {
            LOGE("invalid aac data");
            return;
        }
        p += 7;
        size -= 7;
    }

    uint16_t auHeaderLength = htons(16);      // 16-bits AU headers-length
    uint16_t auHeader = (size << 3) & 0xfff8; // 13bit AU-size/3bit AU-Index/AU-Index-delta

    std::shared_ptr<DataBuffer> rtpPacket = std::make_shared<DataBuffer>(RTP_PACKET_HEADER_DEFAULT_SIZE + 4 + size);

    RtpHeader header;
    uint32_t ts = (uint32_t)(frame->timestamp * (frame->audioInfo.sampleRate / 1000));
    FillRtpHeader(header, 97, ts, true); // fill header

    rtpPacket->Assign(&header, header.GetHeaderLength());
    rtpPacket->Append(&auHeaderLength, sizeof(auHeaderLength));
    rtpPacket->Append(&auHeader, sizeof(auHeader));
    rtpPacket->Append(p, size);

    if (packetizeCallback_) {
        packetizeCallback_(rtpPacket);
    }
}

void RtpDepacketizerAAC::Depacketize(std::shared_ptr<DataBuffer> dataBuffer)
{
    LOGD("size: %zu", dataBuffer->Size());
    dataBuffer->HexDump(32);

    auto frame = std::make_shared<Frame>();
    frame->format = FRAME_FORMAT_AAC;
    frame->audioInfo.channels = channels_;
    frame->audioInfo.nbSamples = nbSamples_;
    frame->audioInfo.sampleRate = sampleRate_;

    int adtsLength = dataBuffer->Size() - 12 - 4;
    ADTSHeader header;
    header.SetChannel(channels_).SetSamplingFrequency(sampleRate_).SetLength(adtsLength + 7);
    frame->Assign(&header, sizeof(header));
    frame->Append(dataBuffer->Data() + 12 + 4, adtsLength);
    depacketizeCallback_(frame);
}

void RtpDepacketizerAAC::SetExtraData(void *extra)
{
    AudioFrameInfo *info = (AudioFrameInfo *)extra;
    channels_ = info->channels;
    nbSamples_ = info->nbSamples;
    sampleRate_ = info->sampleRate;
}
