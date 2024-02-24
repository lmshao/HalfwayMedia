//
// Copyright © 2020-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_FRAME_PIPELINE_H
#define HALFWAY_MEDIA_MEDIA_FRAME_PIPELINE_H

#include "common/frame.h"
#include <shared_mutex>
#include <unordered_map>

class FrameSink;
class FrameSource : public std::enable_shared_from_this<FrameSource> {
public:
    FrameSource() = default;
    virtual ~FrameSource();

    void AddAudioSink(const std::shared_ptr<FrameSink> &sink);
    void RemoveAudioSink(const std::shared_ptr<FrameSink> &sink);

    void AddVideoSink(const std::shared_ptr<FrameSink> &sink);
    void RemoveVideoSink(const std::shared_ptr<FrameSink> &sink);

    uint64_t Id() { return reinterpret_cast<uint64_t>(this); }

    virtual void OnNotify(void *userdata) = 0;

protected:
    void DeliverFrame(const std::shared_ptr<Frame> &frame);
    bool NotifySink(void *userdata);

protected:
    std::shared_mutex audioSinkMutex_;
    std::shared_mutex videoSinkMutex_;
    std::unordered_map<uint64_t, std::weak_ptr<FrameSink>> audioSinks_;
    std::unordered_map<uint64_t, std::weak_ptr<FrameSink>> videoSinks_;
};

class FrameSink : public std::enable_shared_from_this<FrameSink> {
public:
    FrameSink() = default;
    virtual ~FrameSink() = default;

    uint64_t Id() { return reinterpret_cast<uint64_t>(this); }

    virtual void OnFrame(const std::shared_ptr<Frame> &frame) = 0;
    virtual bool OnNotify(void *userdata) = 0;

    void SetAudioSource(const std::shared_ptr<FrameSource> &source);
    void UnsetAudioSource();

    void SetVideoSource(const std::shared_ptr<FrameSource> &source);
    void UnsetVideoSource();

    bool HasAudioSource();
    bool HasVideoSource();

protected:
    void NotifySource(void *userdata);

private:
    std::shared_mutex audioSrcMutex_;
    std::shared_mutex videoSrcMutex_;
    std::weak_ptr<FrameSource> audioSrc_;
    std::weak_ptr<FrameSource> videoSrc_;
};

#endif // HALFWAY_MEDIA_MEDIA_FRAME_PIPELINE_H
