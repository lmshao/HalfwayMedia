//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "Utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
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
