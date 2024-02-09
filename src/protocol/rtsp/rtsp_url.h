//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_POTOCOL_RTSP_URL_H
#define HALFWAY_MEDIA_POTOCOL_RTSP_URL_H

#include "../../common/log.h"
#include <regex>
#include <string>
#include <unordered_map>

class RtspUrl {
public:
    bool Parse(const std::string &url)
    {
        std::regex rtspRegex(
            "^rtsp://(?:([a-zA-Z0-9.-]+):(.+)@)?([a-zA-Z0-9.-]+)(?::([0-9]+))?/([a-zA-Z0-9.-]+)(?:\\?(.+))?");

        std::smatch matches;
        if (!std::regex_match(url, matches, rtspRegex)) {
            LOGE("invalid url (%s)", url.c_str());
            return false;
        }

        username_ = matches[1].str();
        password_ = matches[2].str();
        hostname_ = matches[3].str();
        port_ = stoi(matches[4].str());
        path_ = matches[5].str();

        std::string queryParams = matches[6].str();
        std::regex paramRegex("([^&=]+)=([^&]+)");
        std::sregex_iterator paramIterator(queryParams.begin(), queryParams.end(), paramRegex);
        std::sregex_iterator paramEnd;

        while (paramIterator != paramEnd) {
            std::smatch paramMatch = *paramIterator;
            queryParams_.emplace(paramMatch[1], paramMatch[2]);

            if (username_.empty() && paramMatch[1] == "username") {
                username_ = paramMatch[2];
            }

            if (password_.empty() && paramMatch[1] == "password") {
                password_ = paramMatch[2];
            }

            ++paramIterator;
        }

        url_ = url;
        return true;
    }

    std::string GetUserName() { return username_; }
    std::string GetPassword() { return password_; }
    std::string GetHostName() { return hostname_; }
    uint16_t GetPort() { return port_; }
    std::string GetPath() { return path_; }
    std::unordered_map<std::string, std::string> GetQueryParams() { return queryParams_; }

private:
    std::string url_;
    std::string username_;
    std::string password_;
    std::string hostname_;
    uint16_t port_ = 0;
    std::string path_;
    std::unordered_map<std::string, std::string> queryParams_;
};

#endif // HALFWAY_MEDIA_POTOCOL_RTSP_URL_H