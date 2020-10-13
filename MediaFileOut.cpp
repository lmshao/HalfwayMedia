//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileOut.h"
#include "Utils.h"

MediaFileOut::MediaFileOut(const std::string &url, bool hasAudio, bool hasVideo) : MediaOut(url, hasAudio, hasVideo) {}

MediaFileOut::~MediaFileOut()
{
    logger("~");
};

bool MediaFileOut::isAudioFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
        case FRAME_FORMAT_OPUS:
            return true;
        default:
            return false;
    }
}

bool MediaFileOut::isVideoFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_VP8:
        case FRAME_FORMAT_VP9:
        case FRAME_FORMAT_H264:
        case FRAME_FORMAT_H265:
            return true;
        default:
            return false;
    }
}

const char *MediaFileOut::getFormatName(std::string &url)
{
    return nullptr;
}

uint32_t MediaFileOut::getKeyFrameInterval(void)
{
    return 120000;
}

uint32_t MediaFileOut::getReconnectCount(void)
{
    return 0;
}
