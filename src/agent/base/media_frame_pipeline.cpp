//
// Copyright © 2020-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_frame_pipeline.h"
#include "../../common/log.h"
#include <algorithm>
#include <mutex>

FrameSource::~FrameSource()
{
    {
        std::unique_lock<std::shared_mutex> audioLock(audioSinkMutex_);
        audioSinks_.clear();
    }

    {
        std::unique_lock<std::shared_mutex> videoLock(videoSinkMutex_);
        videoSinks_.clear();
    }
}

void FrameSource::AddAudioSink(const std::shared_ptr<FrameSink> &sink)
{
    LOGD("src(%lu) add audio sink(%lu).", Id(), sink->Id());
    std::unique_lock<std::shared_mutex> lock(audioSinkMutex_);
    audioSinks_.emplace(sink->Id(), sink);
    sink->SetAudioSource(shared_from_this());
}

void FrameSource::RemoveAudioSink(const std::shared_ptr<FrameSink> &sink)
{
    std::unique_lock<std::shared_mutex> lock(audioSinkMutex_);
    auto it = audioSinks_.find(sink->Id());
    if (it != audioSinks_.end()) {
        auto src = it->second.lock();
        if (src) {
            src->UnsetAudioSource();
        }

        LOGD("src(%lu) remove audio sink(%lu).", Id(), sink->Id());
        audioSinks_.erase(it);
    }
}

void FrameSource::AddVideoSink(const std::shared_ptr<FrameSink> &sink)
{
    LOGD("src(%lu) add video sink(%lu).", Id(), sink->Id());
    std::unique_lock<std::shared_mutex> lock(videoSinkMutex_);
    videoSinks_.emplace(sink->Id(), sink);
    sink->SetVideoSource(shared_from_this());
}

void FrameSource::RemoveVideoSink(const std::shared_ptr<FrameSink> &sink)
{
    std::unique_lock<std::shared_mutex> lock(videoSinkMutex_);
    auto it = videoSinks_.find(sink->Id());
    if (it != videoSinks_.end()) {
        auto src = it->second.lock();
        if (src) {
            src->UnsetVideoSource();
        }

        LOGD("src(%lu) remove video sink(%lu).", Id(), sink->Id());
        videoSinks_.erase(it);
    }
}

void FrameSource::DeliverFrame(const std::shared_ptr<Frame> &frame)
{
    if (FrameType(frame->format) == FRAME_FORMAT_AUDIO_BASE) {
        std::shared_lock<std::shared_mutex> lock(audioSinkMutex_);
        for (auto &item : audioSinks_) {
            auto sink = item.second.lock();
            if (sink) {
                sink->OnFrame(frame);
            }
        }
    } else if (FrameType(frame->format) == FRAME_FORMAT_VIDEO_BASE) {
        std::shared_lock<std::shared_mutex> lock(videoSinkMutex_);
        for (auto &item : videoSinks_) {
            auto sink = item.second.lock();
            if (sink) {
                sink->OnFrame(frame);
            }
        }
    } else {
        LOGE("Unknown frame Type");
    }
}
void FrameSink::SetAudioSource(const std::shared_ptr<FrameSource> &source)
{
    LOGD("sink(%lu) set audio source(%lu).", Id(), source->Id());
    std::unique_lock<std::shared_mutex> lock(audioSrcMutex_);
    audioSrc_ = source;
}
void FrameSink::UnsetAudioSource()
{
    LOGD("sink(%lu) unset audio source.", Id());
    std::unique_lock<std::shared_mutex> lock(audioSrcMutex_);
    audioSrc_.reset();
}

void FrameSink::SetVideoSource(const std::shared_ptr<FrameSource> &source)
{
    LOGD("sink(%lu) set video source(%lu).", Id(), source->Id());
    std::unique_lock<std::shared_mutex> lock(videoSrcMutex_);
    videoSrc_ = source;
}

void FrameSink::UnsetVideoSource()
{
    LOGD("sink(%lu) unset video source.", Id());
    std::unique_lock<std::shared_mutex> lock(videoSrcMutex_);
    videoSrc_.reset();
}

bool FrameSink::HasAudioSource()
{
    return audioSrc_.expired();
}

bool FrameSink::HasVideoSource()
{
    return videoSrc_.expired();
}
