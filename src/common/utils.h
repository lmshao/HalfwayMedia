//
// Copyright © 2020-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_UTILS_H
#define HALFWAY_MEDIA_UTILS_H

#include <cstdint>
#include <string>

int64_t Milliseconds();

char *ff_strerror(int errRet);

enum UrlType {
    TYPE_UNKNOWN,
    TYPE_FILE,
    TYPE_RTSP,
};

UrlType DetectUrlType(const std::string &url);

#endif // HALFWAY_MEDIA_UTILS_H
