//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTCP_PACKET_H
#define HALFWAY_MEDIA_PROTOCOL_RTCP_PACKET_H

#include "common/data_buffer.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// clang-format off
enum RtcpType : uint8_t {
    RTCP_SR    = 200, // Sender Report
    RTCP_RR    = 201, // Receiver Report
    RTCP_SDES  = 202, // Source Description
    RTCP_BYE   = 203, // Goodbye
    RTCP_APP   = 204, // Application-defined
    RTCP_RTPFB = 205, // RTCP Feedback Messages, @see https://www.rfc-editor.org/rfc/rfc4585
    RTCP_PSFB  = 206, // Payload-Specific Feedback
    RTCP_XR    = 207, // Extended Reports, @see https://www.rfc-editor.org/rfc/rfc3611
    RTCP_AVB   = 208, // Extended Reports
    RTCP_RSI   = 209, // RTP Statistics
    RTCP_TOKEN = 210
};
// clang-format on

class RtcpHeader {
public:
    uint16_t GetLength() const { return (__builtin_bswap16(length_) + 1) << 2; }
    void SetLength(uint16_t length) { length_ = __builtin_bswap16((length >> 2) - 1); }
    uint32_t GetSSRC() const { return __builtin_bswap32(ssrc_[0]); }
    void SetSSRC(uint32_t ssrc) { ssrc_[0] = __builtin_bswap32(ssrc); }

public:
    uint8_t subtype : 5;
    uint8_t padding : 1;
    uint8_t version : 2;
    uint8_t pt : 8;

private:
    uint16_t length_;
    uint32_t ssrc_[0]; // 0 bit
};

// SR: Sender Report RTCP Packet
// @see https://www.rfc-editor.org/rfc/rfc3550#section-6.4.1
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=SR=200   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         SSRC of sender                        |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// sender |              NTP timestamp, most significant word             |
// info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |             NTP timestamp, least significant word             |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         RTP timestamp                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     sender's packet count                     |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      sender's octet count                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_1 (SSRC of first source)                 |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   1    | fraction lost |       cumulative number of packets lost       |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           extended highest sequence number received           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      interarrival jitter                      |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         last SR (LSR)                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                   delay since last SR (DLSR)                  |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_2 (SSRC of second source)                |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   2    :                               ...                             :
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |                  profile-specific extensions                  |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

struct ReportBlock {
    uint32_t ssrc;
    uint8_t fractionLost;
    uint32_t lostPacketCount : 24;
    uint32_t highestSequence;
    uint32_t jitter;
    uint32_t lastSR;
    uint32_t delaySinceLastSR;
};

class RtcpSR {
public:
    static std::shared_ptr<RtcpSR> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

    struct SenderInfo {
        uint32_t nptTsMSW{};
        uint32_t nptTsLSW{};
        uint32_t rtpTs{};
        uint32_t packetCount{}; // The total number of RTP packets
        uint32_t octetCount{};  // The total number of RTP payload bytes(not including header of padding)

        uint64_t GetNPTTime() const;
        void SetNPTTime(uint64_t utcTime);
    };

public:
    bool padding;
    uint8_t reportCount;
    RtcpType pt = RTCP_SR;
    uint16_t length;
    uint32_t ssrc;
    SenderInfo senderInfo;
    std::vector<ReportBlock> reportBlocks;
};

// RR: Receiver Report RTCP Packet
// @see https://www.rfc-editor.org/rfc/rfc3550#section-6.4.2
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=RR=201   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     SSRC of packet sender                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_1 (SSRC of first source)                 |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   1    | fraction lost |       cumulative number of packets lost       |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           extended highest sequence number received           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      interarrival jitter                      |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         last SR (LSR)                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                   delay since last SR (DLSR)                  |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_2 (SSRC of second source)                |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   2    :                               ...                             :
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |                  profile-specific extensions                  |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RtcpRR {
public:
    static std::shared_ptr<RtcpRR> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

public:
    bool padding;
    uint8_t reportCount;
    RtcpType pt = RTCP_RR;
    uint16_t length;
    uint32_t ssrc;
    std::vector<ReportBlock> reportBlocks;
};

// SDES: Source Description RTCP Packet
// @see https://www.rfc-editor.org/rfc/rfc3550#section-6.5
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    SC   |  PT=SDES=202  |             length            |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_1                          |
//   1    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// chunk  |                          SSRC/CSRC_2                          |
//   2    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           SDES items                          |
//        |                              ...                              |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

// clang-format off
enum SdesType : uint8_t {
    RTCP_SDES_END     = 0,
    RTCP_SDES_CNAME   = 1,
    RTCP_SDES_NAME    = 2,
    RTCP_SDES_EMAIL   = 3,
    RTCP_SDES_PHONE   = 4,
    RTCP_SDES_LOC     = 5,
    RTCP_SDES_TOOL    = 6,
    RTCP_SDES_NOTE    = 7,
    RTCP_SDES_PRIVATE = 8,
};
// clang-format on

struct SdesChunk {
    uint32_t ssrc;
    std::vector<std::pair<SdesType, std::string>> items;
};

class RtcpSDES {
public:
    static std::shared_ptr<RtcpSDES> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

public:
    bool padding;
    uint8_t sourceCount;
    RtcpType pt = RTCP_SDES;
    uint16_t length;
    std::vector<SdesChunk> sdesChunks;
};

// BYE: Goodbye RTCP Packet
// @see https://www.rfc-editor.org/rfc/rfc3550#section-6.6
//        0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |V=2|P|    SC   |   PT=BYE=203  |             length            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                           SSRC/CSRC                           |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       :                              ...                              :
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// (opt) |     length    |               reason for leaving            ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RtcpBYE {
public:
    static std::shared_ptr<RtcpBYE> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

public:
    bool padding;
    uint8_t sourceCount;
    RtcpType pt = RTCP_BYE;
    uint16_t length;
    std::vector<uint32_t> ssrcs;
    std::string reason;
};

// APP: Application-Defined RTCP Packet
// @see https://www.rfc-editor.org/rfc/rfc3550#section-6.7
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P| subtype |   PT=APP=204  |             length            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                           SSRC/CSRC                           |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                          name (ASCII)                         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                   application-dependent data                ...
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RtcpAPP : public RtcpHeader {
public:
    static std::shared_ptr<RtcpAPP> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

public:
    bool padding;
    uint8_t subtype;
    RtcpType pt = RTCP_APP;
    uint16_t length;
};

// Common Packet Format for Feedback Messages
// @see https://www.rfc-editor.org/rfc/rfc4585#section-6.1
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P|   FMT   |  PT=RTPFB=205 |          length               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                  SSRC of packet sender                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                  SSRC of media source                         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    :            Feedback Control Information (FCI)                 :
//    :                                                               :

//  FCI
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |            PID                |             BLP               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RtcpRTPFB {
public:
    static std::shared_ptr<RtcpRTPFB> Parse(const uint8_t *p, size_t length);

    std::shared_ptr<DataBuffer> Generate();

public:
    bool padding;
    uint8_t fmt;
    RtcpType pt = RTCP_RTPFB;
    uint16_t length;
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTCP_PACKET_H