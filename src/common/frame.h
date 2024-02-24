//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_FRAME_H
#define HALFWAY_MEDIA_FRAME_H

#include "data_buffer.h"
#include <cstdint>

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN = 0,
    FRAME_FORMAT_VIDEO_BASE = 100,
    FRAME_FORMAT_I420,
    FRAME_FORMAT_VP8,
    FRAME_FORMAT_VP9,
    FRAME_FORMAT_H264,
    FRAME_FORMAT_H265,
    FRAME_FORMAT_AUDIO_BASE = 200,
    FRAME_FORMAT_PCM_S16,
    FRAME_FORMAT_PCMU,
    FRAME_FORMAT_PCMA,
    FRAME_FORMAT_AAC,
    FRAME_FORMAT_MP3,
};

inline FrameFormat FrameType(FrameFormat format)
{
    return static_cast<FrameFormat>(format / 100 * 100);
}

struct VideoFrameInfo {
    int framerate;
    uint16_t width;
    uint16_t height;
    bool isKeyFrame;
};

struct AudioFrameInfo {
    uint8_t channels;
    uint32_t nbSamples;
    uint32_t sampleRate;
};

class Frame : public DataBuffer {
public:
    template <typename... Args>
    explicit Frame(Args... args) : DataBuffer(args...)
    {
    }

    uint64_t timestamp;
    FrameFormat format;
    union {
        VideoFrameInfo videoInfo;
        AudioFrameInfo audioInfo;
    };
};

#endif // HALFWAY_MEDIA_FRAME_H
