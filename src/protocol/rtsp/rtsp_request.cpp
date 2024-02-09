//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtsp_request.h"
#include "rtsp_common.h"
#include <sstream>
#include <unordered_map>

std::unordered_map<RequestHeader, std::string> RequestHeaderStr = {
    {RequestHeader::INVALID_VALUE, "INVALID-VALUE"},
    // General Header Fields
    {RequestHeader::CSEQ, "CSeq"},
    {RequestHeader::CACHE_CONTROL, "Cache-Control"},
    {RequestHeader::CONNECTION, "Connection"},
    {RequestHeader::DATE, "Date"},
    {RequestHeader::VIA, "Via"},
    // Request Header Fields
    {RequestHeader::USER_AGENT, "User-Agent"},
    {RequestHeader::ACCEPT, "Accept"},
    {RequestHeader::ACCEPT_ENCODING, "Accept-Encoding"},
    {RequestHeader::ACCEPT_LANGUAGE, "Accept-Language"},
    {RequestHeader::AUTHORIZATION, "Authorization"},
    {RequestHeader::FROM, "From"},
    {RequestHeader::IF_MODIFTED_SINCE, "If-Modified-Since"},
    {RequestHeader::RANGE, "Range"},
    {RequestHeader::REFERER, "Referer"},
    // Entity Header Fields
    {RequestHeader::TRANSPORT, "Transport"},
    {RequestHeader::SESSION, "Session"},
    {RequestHeader::ALLOW, "Allow"},
    {RequestHeader::CONTENT_BASE, "Content-Base"},
    {RequestHeader::CONTENT_ENCODING, "Content-Encoding"},
    {RequestHeader::CONTENT_LANGUAGE, "Content-Language"},
    {RequestHeader::CONTENT_LENGTH, "Content-Length"},
    {RequestHeader::CONTENT_LOCATION, "Content-Location"},
    {RequestHeader::CONTENT_TYPE, "Content-Type"},
    {RequestHeader::EXPIRES, "Expires"},
    {RequestHeader::LAST_MODIFIED, "Last-Modified"},
    {RequestHeader::EXTENSION, "--"}};

std::string RtspRequest::Stringify()
{
    std::stringstream ss;
    ss << method_ << SP << requestUrl_ << SP << RTSP_VERSION << CRLF;

    for (auto &header : headers_) {
        ss << RequestHeaderStr[header.first] << ":" << SP << header.second << CRLF;
    }

    if (!messageBody_.empty()) {
        ss << CRLF << messageBody_;
    }

    ss << CRLF;
    return ss.str();
}