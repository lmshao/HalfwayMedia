//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_UTILS_H
#define HALFWAYLIVE_UTILS_H

#define logger(fmt, args...)  print(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)

void print(const char* fname, int line, const char* func, const char* fmt, ...);

#endif //HALFWAYLIVE_UTILS_H
