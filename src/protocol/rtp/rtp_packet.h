//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H
#define HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H

#include "../../common/frame.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <stdint.h>

#define MTU_DEFAULT 1500
#define IP_PACKET_HEADER_SIZE 20
#define UDP_PACKET_HEADER_SIZE 8
#define TCP_PACKET_HEADER_SIZE 20
#define RTP_PACKET_HEADER_DEFAULT_SIZE 12
#define RTP_OVER_UDP_PACKET_PAYLOAD_MAX_SIZE 1460 // 1500-20-8-12
#define RTP_OVER_TCP_PACKET_PAYLOAD_MAX_SIZE 1448 // 1500-20-20-12

//  RTP Fixed Header Fields
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                           timestamp                           |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |           synchronization source (SSRC) identifier            |
//    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//    |            contributing source (CSRC) identifiers             |
//    |                             ....                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//
//  RTP Header Extension
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |      defined by profile       |           length              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                        header extension                       |
//    |                             ....                              |

class RtpHeader {
public:
    RtpHeader()
        : cc_(0), extension_(0), padding_(0), version_(2), payloadType_(0), marker_(0), seqNumber_(0), timestamp_(0),
          ssrc_(0), extId_(0), extLength_(0)
    {
    }

    uint8_t GetVersion() const { return version_; }
    void SetVersion(uint8_t version) { version_ = version; }

    bool GetPadding() const { return padding_ == 1; }
    void SetPadding(bool padding) { padding_ = padding ? 1 : 0; }

    bool GetExtension() const { return extension_ == 1; }
    void SetExtension(bool extension) { extension_ = extension ? 1 : 0; }

    uint8_t GetCC() const { return cc_; }
    void SetCC(uint8_t cc) { cc_ = cc; }

    bool GetMarker() const { return marker_ == 1; }
    void SetMarker(bool marker) { marker_ = marker ? 1 : 0; }

    uint8_t GetPayloadType() const { return payloadType_; }
    void SetPayloadType(uint8_t payloadType) { payloadType_ = payloadType; }

    uint16_t GetSeqNumber() const { return ntohs(seqNumber_); }
    void SetSeqNumber(uint16_t seqNumber) { seqNumber_ = htons(seqNumber); }

    uint32_t GetTimestamp() const { return ntohl(timestamp_); }
    void SetTimestamp(uint32_t timestamp) { timestamp_ = htonl(timestamp); }

    uint32_t GetSSRC() const { return ntohl(ssrc_); }
    void SetSSRC(uint32_t ssrc) { ssrc_ = htonl(ssrc); }

    uint16_t GetExtId() const { return ntohs(extId_); }
    void SetExtId(uint16_t extensionId) { extId_ = htons(extensionId); }

    void SetExtLength(uint16_t extensionLength) { extLength_ = htons(extensionLength); }
    uint16_t GetExtLength() const { return ntohs(extLength_); }

    int GetHeaderLength() const { return 12 + cc_ * 4 + extension_ * (4 + ntohs(extLength_) * 4); }

private:
    uint8_t cc_ : 4;
    uint8_t extension_ : 1;
    uint8_t padding_ : 1;
    uint8_t version_ : 2;
    uint8_t payloadType_ : 7;
    uint8_t marker_ : 1;
    uint16_t seqNumber_;
    uint32_t timestamp_;
    uint32_t ssrc_;
    // uint32_t csrc_[cc_]
    uint16_t extId_;
    uint16_t extLength_;
    // uint32_t extData[extLength_]
};

//----------------------------------------------------------------

class RtpPacketizer {
public:
    virtual ~RtpPacketizer() = default;

    static std::shared_ptr<RtpPacketizer> Create(FrameFormat format);

    virtual void Packetize(const std::shared_ptr<Frame> &frame) = 0;
    void SetCallback(const std::function<void(std::shared_ptr<DataBuffer>)> callback) { packetizeCallback_ = callback; }

    uint32_t GetSSRC()
    {
        if (ssrc_ == 0) {
            ssrc_ = (uint32_t)((uint64_t)this & 0xffffffff);
        }
        return ssrc_;
    }

    void SetSSRC(uint32_t ssrc) { ssrc_ = ssrc; }

    void FillRtpHeader(RtpHeader &header, uint8_t pt, uint32_t ts, bool mark);

protected:
    RtpPacketizer() = default;

protected:
    uint32_t ssrc_ = 0;
    uint16_t seqNumber_ = 0;
    std::function<void(std::shared_ptr<DataBuffer>)> packetizeCallback_;
};

//----------------------------------------------------------------

class RtpDepacketizer {
public:
    virtual ~RtpDepacketizer() = default;

    static std::shared_ptr<RtpDepacketizer> Create(FrameFormat format);

    virtual void Depacketize(std::shared_ptr<DataBuffer> dataBuffer) = 0;
    virtual void SetExtraData(void *extra) {}

    void SetCallback(const std::function<void(std::shared_ptr<Frame>)> callback) { depacketizeCallback_ = callback; }

protected:
    RtpDepacketizer() = default;

protected:
    std::function<void(std::shared_ptr<Frame>)> depacketizeCallback_;
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTP_PACKET_H