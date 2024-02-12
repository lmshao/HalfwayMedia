//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTSP_RESPONSE_H
#define HALFWAY_MEDIA_PROTOCOL_RTSP_RESPONSE_H

#include "../../protocol/rtsp/rtsp_request.h"
#include "rtsp_common.h"
#include <cstdint>
#include <cstdio>
#include <map>
#include <regex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

enum class ResponseHeader : uint8_t {
    INVALID_VALUE = 0x00,
    // General Header Fields
    CSEQ = 0x01,
    CACHE_CONTROL,
    CONNECTION,
    DATE,
    VIA,
    // Response Header Fields
    LOCATION = 0x20,
    PROXY_AUTHENTICATE,
    PUBLIC,
    RETRY_AFTER,
    SERVER,
    VARY,
    WWWW_AUTHENTICATE,
    RANGE,
    RTP_INFO,
    // Entity Header Fields
    TRANSPORT = 0x30,
    SESSION,
    ALLOW,
    CONTENT_BASE,
    CONTENT_ENCODING,
    CONTENT_LANGUAGE,
    CONTENT_LENGTH,
    CONTENT_LOCATION,
    CONTENT_TYPE,
    EXPIRES,
    LAST_MODIFIED,
    EXTENSION
};

class RtspResponse {
public:
    RtspResponse() = default;
    explicit RtspResponse(StatusCode code) : statusCode_(code) {}
    ~RtspResponse() = default;

    StatusCode GetStatusCode() const { return statusCode_; }
    std::string GetStatusMessage() const { return statusMessage_; }

    RtspResponse &SetCSeq(int seq)
    {
        headers_.emplace(ResponseHeader::CSEQ, std::to_string(seq));
        return *this;
    }

    int GetCSeq()
    {
        if (cseq_ == -1) {
            auto cseqStr = headers_.at(ResponseHeader::CSEQ);
            if (!cseqStr.empty()) {
                cseq_ = stoi(cseqStr);
            }
        }

        return cseq_;
    }

    RtspResponse &SetHeaders(ResponseHeader header, std::string value)
    {
        headers_.emplace(header, value);
        return *this;
    }

    std::map<ResponseHeader, std::string> GetHeaders() const { return headers_; }

    std::string GetHeader(ResponseHeader header)
    {
        if (headers_.find(header) != headers_.end()) {
            return headers_.at(header);
        }

        return {};
    }

    RtspResponse &SetMessageBody(const std::string &message)
    {
        messageBody_ = message;
        return *this;
    }

    std::string GetMessageBody() const { return messageBody_; }

    virtual std::string Stringify();
    bool Parse(const std::string &response);

private:
    int cseq_ = -1;
    StatusCode statusCode_;
    std::string statusMessage_;
    std::string messageBody_;
    std::map<ResponseHeader, std::string> headers_;
};

class RtspResponseOptions : public RtspResponse {
public:
    explicit RtspResponseOptions(StatusCode code) : RtspResponse(code) {}
    explicit RtspResponseOptions(const RtspResponse &response) : RtspResponse(response) {}

    std::vector<std::string> GetPublic();
};

class RtspResponseDescribe : public RtspResponse {
public:
    explicit RtspResponseDescribe(StatusCode code) : RtspResponse(code) {}
    explicit RtspResponseDescribe(const RtspResponse &response) : RtspResponse(response) {}

    std::string GetContentBaseUrl();
    std::string GetContentType();
    int GetContentLength();
};

class RtspResponseSetup : public RtspResponse {
public:
    explicit RtspResponseSetup(StatusCode code) : RtspResponse(code) {}
    explicit RtspResponseSetup(const RtspResponse &response) : RtspResponse(response) {}

    RtspResponse &SetTransport(std::string transport);
    std::string GetTransport();
    std::pair<uint16_t, uint16_t> GetServerPort();
    std::string GetSession();
    int GetTimeout();
};

class RtspResponsePlay : public RtspResponse {
public:
    explicit RtspResponsePlay(StatusCode code) : RtspResponse(code) {}
    explicit RtspResponsePlay(const RtspResponse &response) : RtspResponse(response) {}

    RtspResponse &SetRange(std::string range);

    std::string GetRtpInfo() { return GetHeader(ResponseHeader::RTP_INFO); }
};

class RtspResponseTeardown : public RtspResponse {
public:
    explicit RtspResponseTeardown(StatusCode code) : RtspResponse(code) {}
    explicit RtspResponseTeardown(const RtspResponse &response) : RtspResponse(response) {}
};
#endif // HALFWAY_MEDIA_PROTOCOL_RTSP_RESPONSE_H