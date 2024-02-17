//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H264_H
#define HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H264_H

#include "../../common/data_buffer.h"
#include "rtp_packet.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

//
//  Network Abstraction Layer Unit Types
//
//    +---------------+
//    |0|1|2|3|4|5|6|7|
//    +-+-+-+-+-+-+-+-+
//    |F|NRI|  Type   |
//    +---------------+

#define NALU_TYPE(octet) (((octet) & 0x1f))

enum NaluType {
    NALU_RESERVED = 0,
    NALU_SLICE_NON_IDR = 1, // Coded slice of a non-IDR picture, P frame
    NALU_SLICE_A = 2,       // Coded slice data partition A, B frame
    NALU_SLICE_B = 3,       // Coded slice data partition B
    NALU_SLICE_C = 4,       // Coded slice data partition C
    NALU_IDR = 5,           // Coded slice of an IDR picture
    NALU_SEI = 6,           // Supplemental enhancement information
    NALU_SPS = 7,           // Sequence parameter set
    NALU_PPS = 8,           // Picture parameter set
    NALU_AUD = 9,           // Access unit delimiter
    NALU_EOSEQ = 10,        // End of sequence
    NALU_EOSTREAM = 11,     // End of stream
    NALU_STAP_A = 24,
    NALU_STAP_B = 25,
    NALU_MTAP16 = 26,
    NALU_MTAP24 = 27,
    NALU_FU_A = 28,
    NALU_FU_B = 29,
};

//  FU header
//
//  +---------------+
//   |0|1|2|3|4|5|6|7|
//   +-+-+-+-+-+-+-+-+
//   |S|E|R|  Type   |
//   +---------------+
//

struct FUHeader {
    uint8_t type : 5;
    uint8_t reserved : 1;
    uint8_t end : 1;
    uint8_t start : 1;
};

class RtpPacketizerH264 : public RtpPacketizer {
public:
    void Packetize(const std::shared_ptr<Frame> &frame) override;

private:
    void MakeIDRPacket(const uint8_t *data, size_t length, int64_t ts);
    void MakeSinglePacket(const uint8_t *data, size_t length, int64_t ts);
    void MakeStapAPacket(std::vector<std::pair<const uint8_t *, size_t>>, int64_t ts);
    void MakeFuAPacket(const uint8_t *data, size_t length, int64_t ts);

private:
    std::unique_ptr<DataBuffer> sps_;
    std::unique_ptr<DataBuffer> pps_;
};

class RtpDepacketizerH264 : public RtpDepacketizer {
public:
    void DepacketizeInner(std::shared_ptr<DataBuffer> dataBuffer) override;

private:
    void HandleSinglePacket(std::shared_ptr<DataBuffer> dataBuffer);
    void HandleFuAPacket(std::shared_ptr<DataBuffer> dataBuffer);

    void PopCache();

private:
    uint16_t lastSeq_;
    uint32_t lastTs_;
    int cacheDataLength_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<DataBuffer>> cache_;
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H264_H