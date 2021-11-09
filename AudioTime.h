//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYMEDIA_AUDIOTIME_H
#define HALFWAYMEDIA_AUDIOTIME_H

#include <cstdint>
class AudioTime {
  public:
    static int64_t currentTime(void);  // Millisecond
    static void setTimestampOffset(uint32_t offset);

  private:
    static uint32_t sTimestampOffset;
};

#endif  // HALFWAYMEDIA_AUDIOTIME_H
