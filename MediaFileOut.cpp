//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileOut.h"
#include <unistd.h>
#include "Utils.h"

MediaFileOut::MediaFileOut(const std::string &url, bool hasAudio, bool hasVideo)
  : MediaOut(url, hasAudio, hasVideo)
{
}

MediaFileOut::~MediaFileOut()
{
    logger("~");
};

[[noreturn]] void MediaFileOut::sendLoop()
{
    for (;;) {
        logger("--");
        sleep(1);
    }
}

bool MediaFileOut::isAudioFormatSupported(FrameFormat format)
{
    return false;
}

bool MediaFileOut::isVideoFormatSupported(FrameFormat format)
{
    return false;
}

const char *MediaFileOut::getFormatName(std::string &url)
{
    return nullptr;
}

uint32_t MediaFileOut::getKeyFrameInterval(void)
{
    return 0;
}

uint32_t MediaFileOut::getReconnectCount(void)
{
    return 0;
}
