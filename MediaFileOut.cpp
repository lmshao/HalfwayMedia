//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileOut.h"
#include <unistd.h>
#include "Utils.h"

MediaFileOut::MediaFileOut(const std::string &url, bool hasAudio, bool hasVideo) :
MediaOut(url, hasAudio, hasVideo) {

}

MediaFileOut::~MediaFileOut() = default;

[[noreturn]] void MediaFileOut::sendLoop()
{
    for (;;) {
        logger("--");
        sleep(1);
    }
}
