//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_UTILS_H
#define HALFWAYLIVE_UTILS_H

#include <stdint.h>
#define logger(fmt, args...) print(__FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ##args)

// #define logger(fmt, args...)

void print(const char *fname, int line, const char *func, const char *fmt, ...);

char *ff_err2str(int errRet);

#define DUMP_HEX(x, y)      \
    {                       \
        printf("%s: ", #x); \
        dumpHex(x, y);      \
    }

// #define DUMP_HEX(x, y)

char *dumpHex(const uint8_t *ptr, int lenght);

void setTimestampOffset(uint32_t offset);

int64_t startTime();  // ms

int64_t currentTime();  // ms

#endif  // HALFWAYLIVE_UTILS_H
