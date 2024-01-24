//
// Copyright © 2020-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "utils.h"
#include <chrono>
#include <stdio.h>
#include <string.h>

extern "C" {
#include <libavutil/error.h>
}

int64_t Milliseconds()
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

char *ff_strerror(int errRet)
{
    static char errBuff[64];
    return av_make_error_string(errBuff, 64, errRet);
}