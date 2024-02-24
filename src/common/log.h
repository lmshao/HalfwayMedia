//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_LOG_H
#define HALFWAY_MEDIA_LOG_H

#include <string>

std::string Time();

#define FILENAME_ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef RELEASE

#define LOGD(fmt, ...)
#define LOGW(fmt, ...)

#else

#define LOGD(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        printf("%s - %s:%d - %s: " fmt "\n", Time().c_str(), FILENAME_, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__); \
    } while (0);

#define LOGW(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        printf("\033[0;33m%s - %s:%d - %s: " fmt "\033[0m\n", Time().c_str(), FILENAME_, __LINE__,                     \
               __PRETTY_FUNCTION__, ##__VA_ARGS__);                                                                    \
    } while (0);

#endif // RELEASE

#define LOGE(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        printf("\033[0;31m%s - %s:%d - %s: " fmt "\033[0m\n", Time().c_str(), FILENAME_, __LINE__,                     \
               __PRETTY_FUNCTION__, ##__VA_ARGS__);                                                                    \
    } while (0);

#endif // HALFWAY_MEDIA_LOG_H