//
// Copyright © 2020-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "log.h"

#include <chrono>

std::string Time()
{
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    char date[24] = {0};
    struct tm *ptm = localtime(&tt);
    sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d.%03d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(ms % 1000));
    return date;
}