//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "Utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

extern "C" {
#include <libavutil/error.h>
}

void print(const char *fname, int line, const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char *ptr = strrchr(fname, '/');
    if (ptr) {
        fname = ptr + 1;
    }
    printf("%s(%d), %s: ", fname, line, func);
    vprintf(fmt, ap);
    printf("\r\n");
    va_end(ap);
}

char *ff_err2str(int errRet)
{
    static char errBuff[500];
    av_strerror(errRet, (char *)(&errBuff), 500);
    return errBuff;
}

char *dumpHex(const uint8_t *ptr, int length)
{
    static char dd[100];
    memset(dd, 0, sizeof(dd));

    int i;
    for (i = 0; i < length; ++i) {
        sprintf(dd + i * 2, "%.2x", *(ptr + i));
    }

    printf("%s\n", dd);
    return dd;
}

static int64_t startTimestamp = 0;
static bool initTimeFlag = false;

void initStartTimestamp()
{
    static timeval time{};
    gettimeofday(&time, nullptr);
    startTimestamp = ((time.tv_sec * 1000) + (time.tv_usec / 1000));
    initTimeFlag = true;
}

int64_t startTime()
{
    if (!initTimeFlag) {
        initStartTimestamp();
    }
    return startTimestamp;
}

int64_t currentTime()
{
    if (!initTimeFlag) {
        initStartTimestamp();
    }
    static timeval time{};
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}