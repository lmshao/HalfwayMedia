//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTSP_REQUEST_H
#define HALFWAY_MEDIA_PROTOCOL_RTSP_REQUEST_H

#include "rtsp_common.h"
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

enum class RequestHeader : uint8_t {
    INVALID_VALUE = 0x00,
    // General Header Fields
    CSEQ = 0x01,
    CACHE_CONTROL,
    CONNECTION,
    DATE,
    VIA,
    // Request Header Fields
    USER_AGENT = 0x10,
    ACCEPT,
    ACCEPT_ENCODING,
    ACCEPT_LANGUAGE,
    AUTHORIZATION,
    FROM,
    IF_MODIFTED_SINCE,
    RANGE,
    REFERER,
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

class RtspRequest {
public:
    RtspRequest() = delete;
    explicit RtspRequest(std::string method, std::string url = "*") : method_(method), requestUrl_(url) {}
    ~RtspRequest() = default;

    RtspRequest &SetMethod(const std::string &method)
    {
        method_ = method;
        return *this;
    }

    RtspRequest &SetUrl(const std::string &url)
    {
        requestUrl_ = url;
        return *this;
    }

    RtspRequest &SetCSeq(int seq)
    {
        headers_.emplace(RequestHeader::CSEQ, std::to_string(seq));
        return *this;
    }

    RtspRequest &SetUserAgent(const std::string &userAgent)
    {
        headers_.emplace(RequestHeader::USER_AGENT, userAgent);
        return *this;
    }

    RtspRequest &SetSession(const std::string &session)
    {
        headers_.emplace(RequestHeader::SESSION, session);
        return *this;
    }

    RtspRequest &SetHeaders(RequestHeader header, std::string value)
    {
        headers_.emplace(header, value);
        return *this;
    }

    RtspRequest &SetMessageBody(const std::string &message)
    {
        messageBody_ = message;
        return *this;
    }

    virtual std::string Stringify();

private:
    int cseq_;
    std::string method_;
    std::string requestUrl_ = "*";
    std::string messageBody_;
    std::map<RequestHeader, std::string> headers_;
};

class RtspRequestOptions : public RtspRequest {
public:
    explicit RtspRequestOptions(std::string url = "*") : RtspRequest(OPTIONS, url) {}
};

class RtspRequestDescribe : public RtspRequest {
public:
    explicit RtspRequestDescribe(std::string url = "*") : RtspRequest(DESCRIBE, url) {}
    RtspRequest &SetAccept(std::string accept)
    {
        SetHeaders(RequestHeader::ACCEPT, accept);
        return *this;
    }
};

class RtspRequestSetup : public RtspRequest {
public:
    explicit RtspRequestSetup(std::string url = "*") : RtspRequest(SETUP, url) {}

    RtspRequest &SetTransport(uint16_t rtpPort, uint16_t rtcpPort = 0)
    {
        std::stringstream ss;
        ss << "RTP/AVP;unicast;client_port=" << rtpPort << "-" << (rtcpPort == 0 ? rtpPort + 1 : rtcpPort);
        SetHeaders(RequestHeader::TRANSPORT, ss.str());
        return *this;
    }
};

class RtspRequestPlay : public RtspRequest {
public:
    explicit RtspRequestPlay(std::string url = "*") : RtspRequest(PLAY, url) {}
    RtspRequest &SetRange(std::string range)
    {
        SetHeaders(RequestHeader::RANGE, range);
        return *this;
    }
};

class RtspRequestTeardown : public RtspRequest {
public:
    explicit RtspRequestTeardown(std::string url = "*") : RtspRequest(TEARDOWN, url) {}
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTSP_REQUEST_H