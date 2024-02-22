//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_sink.h"
#include <memory>

void MediaSink::SetMediaInfo(VideoFrameInfo *videoInfo, AudioFrameInfo *audioInfo)
{
    if (videoInfo) {
        if (!videoInfo_) {
            videoInfo_ = std::make_unique<VideoFrameInfo>();
        }
        videoInfo_->framerate = videoInfo->framerate;
        videoInfo_->width = videoInfo->width;
        videoInfo_->height = videoInfo->height;
        videoInfo_->isKeyFrame = videoInfo->isKeyFrame;
    }

    if (audioInfo) {
        if (!audioInfo_) {
            audioInfo_ = std::make_unique<AudioFrameInfo>();
        }
        audioInfo_->channels = audioInfo->channels;
        audioInfo_->nbSamples = audioInfo->nbSamples;
        audioInfo_->sampleRate = audioInfo->sampleRate;
    }
}
