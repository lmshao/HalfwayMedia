//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "VideoEncoder.h"
#include "Utils.h"

VideoEncoder::VideoEncoder(FrameFormat format) {}

VideoEncoder::~VideoEncoder() {}

void VideoEncoder::onFrame(const Frame &frame) {}

int32_t VideoEncoder::generateStream(uint32_t width, uint32_t height, FrameDestination *dest)
{
    std::unique_lock<std::shared_mutex> uniqueLock(_mutex);
    logger("generateStream: {.width=%d, .height=%d, .frameRate=d, .bitrateKbps=d, .keyFrameIntervalSeconds=d}", width,
           height);

    return 0;
}

void VideoEncoder::degenerateStream(int32_t streamId) {}
