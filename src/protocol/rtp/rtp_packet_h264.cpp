//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_packet_h264.h"
#include "../../common/log.h"
#include "../../common/utils.h"
#include "rtp_packet.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <vector>

static int PrefixSize(const uint8_t *p)
{
    if (p[0] == 0x00 && p[1] == 0x00) {
        if (p[2] == 0x01) {
            return 3;
        } else if (p[2] == 0x00 && p[3] == 0x01) {
            return 4;
        }
    }
    return 0;
}

static const uint8_t *FindNextNal(const uint8_t *start, const uint8_t *end, int &nalSize)
{
    while (start <= end - 3) {
        if (start[0] == 0x00 && start[1] == 0x00) {
            if (start[2] == 0x01) {
                nalSize = 3;
                return start;
            } else if (start[2] == 0x00 && start[3] == 0x01) {
                nalSize = 4;
                return start;
            }
        }
        ++start;
    }
    return end;
}

static std::vector<std::tuple<const uint8_t *, size_t, int>> SplitH264Frame(const uint8_t *data, size_t length)
{
    std::vector<std::tuple<const uint8_t *, size_t, int>> frames;
    const uint8_t *last = nullptr;
    const uint8_t *current = data;
    const uint8_t *end = data + length;
    int prefixLength = 0, lastPrefixLength = 0;

    while (current < end) {
        current = FindNextNal(current, end, prefixLength);
        if (last) {
            frames.emplace_back(last, current - last, lastPrefixLength);
        }
        last = current;
        current += prefixLength;
        lastPrefixLength = prefixLength;
    }

    return frames;
}

void RtpPacketizerH264::Packetize(const std::shared_ptr<Frame> &frame)
{
    LOGD("enter size: %zu", frame->Size());

    auto frames = SplitH264Frame(frame->Data(), frame->Size());

    LOGW("...");
    for (auto &f : frames) {
        const uint8_t *nalu = std::get<0>(f);
        size_t size = std::get<1>(f);
        int prefixLength = std::get<2>(f);

        switch (NALU_TYPE(nalu[prefixLength])) {
            case NALU_SPS:
                if (sps_ == nullptr) {
                    sps_ = std::make_unique<DataBuffer>();
                    sps_->Assign(nalu + prefixLength, size - prefixLength);
                }
                break;
            case NALU_PPS:
                if (pps_ == nullptr) {
                    pps_ = std::make_unique<DataBuffer>();
                    pps_->Assign(nalu + prefixLength, size - prefixLength);
                }
                break;
            case NALU_IDR:
                MakeIDRPacket(nalu + prefixLength, size - prefixLength, frame->timestamp);
                break;
            default:
                MakeFuAPacket(nalu + prefixLength, size - prefixLength, frame->timestamp);
                break;
        }
    }
    LOGD("leave");
}

void RtpPacketizerH264::MakeIDRPacket(const uint8_t *data, size_t length, int64_t ts)
{
    if (!sps_->Empty() && !pps_->Empty()) {
        std::vector<std::pair<const uint8_t *, size_t>> nalus;
        nalus.emplace_back(sps_->Data(), sps_->Size());
        nalus.emplace_back(pps_->Data(), pps_->Size());

        if (1 + 2 + sps_->Size() + 2 + pps_->Size() + 2 + length <= RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE) {
            nalus.emplace_back(data, length);
            MakeStapAPacket(nalus, ts);
        } else {
            MakeStapAPacket(nalus, ts);
            MakeFuAPacket(data, length, ts);
        }
    } else {
        MakeFuAPacket(data, length, ts);
    }
}

//  RTP payload format for single NAL unit packet
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |F|NRI|  Type   |                                               |
//  +-+-+-+-+-+-+-+-+                                               |
//  |                                                               |
//  |               Bytes 2..n of a single NAL unit                 |
//  |                                                               |
//  |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                               :...OPTIONAL RTP padding        |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//

void RtpPacketizerH264::MakeSinglePacket(const uint8_t *data, size_t length, int64_t ts)
{
    if (length > RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE) {
        LOGE("data size [%zu] exceeded max packet payload size", length);
        return;
    }

    std::shared_ptr<DataBuffer> rtpPacket = std::make_shared<DataBuffer>(length + RTP_PACKET_HEADER_DEFAULT_SIZE);
    RtpHeader header;
    int64_t now = Milliseconds();
    uint32_t myts = (uint32_t)(now * (90000 / 1000));
    FillRtpHeader(header, 96, myts, true); // fill header
    rtpPacket->Assign(&header, header.GetHeaderLength());
    rtpPacket->Append(data, length);

    if (packetizeCallback_) {
        packetizeCallback_(rtpPacket);
    }
}

//  STAP-A packet containing two single-time aggregation units
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                          RTP Header                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                         NALU 1 Data                           |
//  :                                                               :
//  +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |               | NALU 2 Size                   | NALU 2 HDR    |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                         NALU 2 Data                           |
//  :                                                               :
//  |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                               :...OPTIONAL RTP padding        |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//

void RtpPacketizerH264::MakeStapAPacket(std::vector<std::pair<const uint8_t *, size_t>> nalus, int64_t ts)
{
    size_t packetSize = 1;
    for (auto &nal : nalus) {
        packetSize += (2 + nal.second);
    }

    if (packetSize > RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE) {
        LOGE("data size [%zu] exceeded max packet payload size", packetSize);
        return;
    }

    std::shared_ptr<DataBuffer> rtpPacket = std::make_shared<DataBuffer>(packetSize + RTP_PACKET_HEADER_DEFAULT_SIZE);
    RtpHeader header;
    int64_t now = Milliseconds();
    uint32_t myts = (uint32_t)(now * (90000 / 1000));
    FillRtpHeader(header, 96, myts, true); // fill header
    rtpPacket->Assign(&header, header.GetHeaderLength());

    uint8_t stapAHeader = ((NALU_STAP_A & 0x1f) | (nalus[0].first[0] & 0x60)); // TYPE & NRI
    rtpPacket->Append(stapAHeader);

    for (auto &nal : nalus) {
        uint16_t nalSize = htons(nal.second);
        rtpPacket->Append(&nalSize, sizeof(nalSize));
        rtpPacket->Append(nal.first, nal.second);
        LOGD("append packet, size: %lu", nal.second);
    }

    if (packetizeCallback_) {
        packetizeCallback_(rtpPacket);
    }
}

//  RTP payload format for FU-A
//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | FU indicator  |   FU header   |                               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
//  |                                                               |
//  |                         FU payload                            |
//  |                                                               |
//  |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                               :...OPTIONAL RTP padding        |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//  FU indicator octet
//
//  +---------------+
//  |0|1|2|3|4|5|6|7|
//  +-+-+-+-+-+-+-+-+
//  |F|NRI|  Type   |
//  +---------------+
//
//  FU header
//
//  +---------------+
//   |0|1|2|3|4|5|6|7|
//   +-+-+-+-+-+-+-+-+
//   |S|E|R|  Type   |
//   +---------------+
//

void RtpPacketizerH264::MakeFuAPacket(const uint8_t *data, size_t length, int64_t ts)
{
    LOGD("nalu %02x-%02x-%02x-%02x, length: %zu", data[0], data[1], data[2], data[3], length);
    if (length <= RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE) {
        MakeSinglePacket(data, length, ts);
        return;
    }

    RtpHeader header;
    int64_t now = Milliseconds();
    uint32_t myts = (uint32_t)(now * (90000 / 1000));

    const uint8_t *p = data + 1;
    uint8_t fuIndicator = (data[0] & 0x60) | (NALU_FU_A & 0x1f); // NRI & TYPE
    int segmentLength = RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE - 2;
    int segmentCount = (length - 1 + segmentLength - 1) / segmentLength;

    for (size_t i = 0; i < segmentCount; i++) {
        std::shared_ptr<DataBuffer> rtpPacket = std::make_shared<DataBuffer>();
        rtpPacket->SetCapacity(RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE + RTP_PACKET_HEADER_DEFAULT_SIZE);

        if (i == segmentCount - 1) {
            FillRtpHeader(header, 96, myts, true); // fill header
        } else {
            FillRtpHeader(header, 96, myts, false); // fill header
        }
        rtpPacket->Assign(&header, header.GetHeaderLength());
        rtpPacket->Append(fuIndicator);

        FUHeader fuHeader{};
        fuHeader.type = data[0] & 0x1f;

        if (i == segmentCount - 1) {
            fuHeader.end = 1;
            rtpPacket->Append(&fuHeader, 1);
            rtpPacket->Append(p + segmentLength * i, (length - 1) % segmentLength);
            if (packetizeCallback_) {
                packetizeCallback_(rtpPacket);
            }
            break;
        }

        if (i == 0) {
            fuHeader.start = 1;
        }

        rtpPacket->Append(&fuHeader, 1);
        rtpPacket->Append(p + segmentLength * i, segmentLength);
        if (packetizeCallback_) {
            packetizeCallback_(rtpPacket);
        }
    }
}