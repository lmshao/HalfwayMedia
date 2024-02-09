//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTSP_SDP_H
#define HALFWAY_MEDIA_PROTOCOL_RTSP_SDP_H

#include "../../common/data_buffer.h"
#include "../../common/log.h"
#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

/// SDP: Session Description Protocol
/// [[RFC 8866](https://datatracker.ietf.org/doc/html/rfc8866)]

enum MediaType {
    VIDEO,
    AUDIO,
    APPLICATION,
    DATA,
    CONTROL
};

struct SessionOrigin {
    bool Parse(const std::string &origin);

    uint32_t version;

    std::string netType;
    std::string username;
    std::string addrType;
    std::string sessionId;
    std::string unicastAddr;
};

struct MediaDescription {
public:
    std::string GetTrackId();

    std::string GetAudioConfig();
    bool GetAudioConfig(int &pt, std::string &format, int &samplingRate, int &channels);

    bool GetVideoConfig(int &pt, std::string &format, int &clockCycle);
    std::shared_ptr<DataBuffer> GetVideoSps();
    std::shared_ptr<DataBuffer> GetVideoPps();
    std::pair<int, int> GetVideoSize();

private:
    bool ParseVideoSpsPps();
    int32_t GetSe(uint8_t *buf, uint32_t nLen, uint32_t &pos);
    int32_t GetUe(const uint8_t *buf, uint32_t nLen, uint32_t &pos);
    int32_t GetU(uint8_t bitCount, const uint8_t *buf, uint32_t &pos);

    void ExtractNaluRbsp(uint8_t *buf, uint32_t *bufSize);

public:
    MediaType type;
    uint16_t port;
    std::string transportType;
    int payloadType;
    std::string name;                    // m=
    std::string title;                   // i=
    std::string connectionInfo;          // c=
    std::string bandwidth;               // b=
    std::vector<std::string> attributes; // a=

    std::shared_ptr<DataBuffer> sps_;
    std::shared_ptr<DataBuffer> pps_;
};

struct SessionTime {
    // r=<repeat interval> <active duration> <offsets from start-time>
    std::vector<std::string> repeat;
    // t=<start-time> <stop-time>
    std::pair<uint64_t, uint64_t> time{UINT64_MAX, UINT64_MAX};
    // z=<adjustment time> <offset> <adjustment time> <offset> ...
    std::string zone;
};

class RtspSdp {
public:
    bool Parse(const std::string &sdp);

    std::shared_ptr<MediaDescription> GetVideoTrack() { return videoTrack_; }
    std::shared_ptr<MediaDescription> GetAudioTrack() { return audioTrack_; }

private:
    int version_ = 0;            // v=
    SessionOrigin origin_;       // o=, originator and session identifier
    std::string sessionName_;    // s=
    std::string sessionTitle_;   // i=
    std::string connectionInfo_; // c=
    std::string time_;
    std::vector<std::string> attributes_;
    std::shared_ptr<MediaDescription> videoTrack_;
    std::shared_ptr<MediaDescription> audioTrack_;
};
#endif // HALFWAY_MEDIA_PROTOCOL_RTSP_SDP_H