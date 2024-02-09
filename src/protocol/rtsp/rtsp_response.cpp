//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtsp_response.h"
#include "../../common/log.h"
#include "rtsp_common.h"
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

std::unordered_map<ResponseHeader, std::string> ResponseHeaderStr = {
    {ResponseHeader::INVALID_VALUE, "INVALID-VALUE"},
    // General Header Fields
    {ResponseHeader::CSEQ, "CSeq"},
    {ResponseHeader::CACHE_CONTROL, "Cache-Control"},
    {ResponseHeader::CONNECTION, "Connection"},
    {ResponseHeader::DATE, "Date"},
    {ResponseHeader::VIA, "Via"},
    // Response Header Fields
    {ResponseHeader::LOCATION, "Location"},
    {ResponseHeader::PROXY_AUTHENTICATE, "Proxy-Authenticate"},
    {ResponseHeader::PUBLIC, "Public"},
    {ResponseHeader::RETRY_AFTER, "Retry-After"},
    {ResponseHeader::SERVER, "Server"},
    {ResponseHeader::VARY, "Vary"},
    {ResponseHeader::WWWW_AUTHENTICATE, "WWW-Authenticate"},
    {ResponseHeader::RANGE, "Range"},
    {ResponseHeader::RTP_INFO, "RTP-Info"},

    // Entity Header Fields
    {ResponseHeader::TRANSPORT, "Transport"},
    {ResponseHeader::SESSION, "Session"},
    {ResponseHeader::ALLOW, "Allow"},
    {ResponseHeader::CONTENT_BASE, "Content-Base"},
    {ResponseHeader::CONTENT_ENCODING, "Content-Encoding"},
    {ResponseHeader::CONTENT_LANGUAGE, "Content-Language"},
    {ResponseHeader::CONTENT_LENGTH, "Content-Length"},
    {ResponseHeader::CONTENT_LOCATION, "Content-Location"},
    {ResponseHeader::CONTENT_TYPE, "Content-Type"},
    {ResponseHeader::EXPIRES, "Expires"},
    {ResponseHeader::LAST_MODIFIED, "Last-Modified"},
    {ResponseHeader::EXTENSION, "--"}};

inline static std::string ToLower(const std::string &s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

static ResponseHeader FindKeyByValue(const std::string &value)
{
    ResponseHeader header = ResponseHeader::INVALID_VALUE;
    for (auto &it : ResponseHeaderStr) {
        if (ToLower(it.second) == ToLower(value)) {
            header = it.first;
            break;
        }
    }
    return header;
}

std::string RtspResponse::Stringify()
{
    std::stringstream ss;
    ss << RTSP_VERSION << SP << statusCode_ << SP << StatusCodeReason[statusCode_] << CRLF;

    for (auto &header : headers_) {
        ss << ResponseHeaderStr[header.first] << ":" << SP << header.second << CRLF;
    }

    if (!messageBody_.empty()) {
        ss << CRLF << messageBody_;
    }

    ss << CRLF;
    return ss.str();
}

bool RtspResponse::Parse(const std::string &response)
{
    const std::regex statusLineRegex(R"((^RTSP/\d.\d) (\d+) (\S.*))");
    const std::regex headerRegex(R"((\S+): (\S.*))");

    std::smatch statusLineMatch;
    if (!std::regex_search(response, statusLineMatch, statusLineRegex)) {
        LOGE("Failed to parse RTSP response status line");
        return false;
    }

    if (statusLineMatch[1].compare(RTSP_VERSION) != 0) {
        LOGE("Unsupported RTSP version (%s)", statusLineMatch[1].str().c_str());
        return false;
    }

    statusCode_ = (StatusCode)std::stoi(statusLineMatch[2]);
    statusMessage_ = statusLineMatch[3];

    auto headerStringEnd = response.end();
    auto blankLinePos = response.find("\r\n\r\n");
    if (blankLinePos != std::string::npos) {
        headerStringEnd = (response.begin() + blankLinePos);
        messageBody_ = response.substr(blankLinePos + 4);
    }

    auto headerBegin = std::sregex_iterator(response.begin(), headerStringEnd, headerRegex);
    auto headerEnd = std::sregex_iterator();
    for (auto it = headerBegin; it != std::sregex_iterator(); ++it) {
        auto x = FindKeyByValue((*it)[1]);
        if (x != ResponseHeader::INVALID_VALUE) {
            headers_.emplace(x, (*it)[2]);
        } else {
            LOGE("Unrecognizable header field: %s", (*it)[1].str().c_str());
        }
    }

    return true;
}

std::vector<std::string> RtspResponseOptions::GetPublic()
{
    std::regex regex("\\s*,\\s*");

    std::string publics = GetHeader(ResponseHeader::PUBLIC);
    std::vector<std::string> tokens(std::sregex_token_iterator(publics.begin(), publics.end(), regex, -1),
                                    std::sregex_token_iterator());

    return tokens;
}

std::string RtspResponseDescribe::GetContentBaseUrl()
{
    return GetHeader(ResponseHeader::CONTENT_BASE);
}

std::string RtspResponseDescribe::GetContentType()
{
    return GetHeader(ResponseHeader::CONTENT_TYPE);
}

int RtspResponseDescribe::GetContentLength()
{
    std::string length = GetHeader(ResponseHeader::CONTENT_LENGTH);
    if (length.empty()) {
        return -1;
    }

    return std::stoi(length);
}