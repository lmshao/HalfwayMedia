//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "Utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void print(const char* fname, int line, const char* func, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char* ptr = strrchr(fname, '/');
    if (ptr) {
        fname = ptr + 1;
    }
    printf("%s(%d), %s: ", fname, line, func);
    vprintf(fmt, ap);
    printf("\r\n");
    va_end(ap);
}