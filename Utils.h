//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_UTILS_H
#define HALFWAYLIVE_UTILS_H

#include <stdint.h>
#include <string.h>

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

char *dumpHex(const uint8_t *ptr, int length);

int64_t startTime();  // ms

int64_t currentTime();  // ms

struct DataBuffer {
    uint8_t *data;
    int length;

    explicit DataBuffer(int length) : length(length) { data = new uint8_t[length]; }

    ~DataBuffer() { delete[] data; }

    void reset(int size)
    {
        delete[] data;
        length = size;
        data = new uint8_t[length];
    }

    void clear()
    {
        if (data)
            memset(data, 0, length);
    }
};

#endif  // HALFWAYLIVE_UTILS_H
