//
// Created by lmshao on 2021/11/6.
//

#include "AudioTime.h"

#include <sys/time.h>

uint32_t AudioTime::sTimestampOffset = 0;

void AudioTime::setTimestampOffset(uint32_t offset)
{
    sTimestampOffset = offset;
}

int64_t AudioTime::currentTime(void)
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - sTimestampOffset;
}