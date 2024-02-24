//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_SINK_H
#define HALFWAY_MEDIA_MEDIA_SINK_H

#include "common/frame.h"
#include "event_definition.h"
#include "media_frame_pipeline.h"
#include <memory>

class MediaSink : public FrameSink {
public:
    MediaSink() = default;
    ~MediaSink() override = default;

    void SetMediaInfo(VideoFrameInfo *videoInfo, AudioFrameInfo *audioInfo);

    [[deprecated("This function will be deprecated")]] bool Notify(AgentEvent event);

    void NotifySource(AgentEvent event);

private:
    virtual bool OnNotify(void *userdata) override;

    virtual bool Init() = 0;
    virtual bool Stop() { return true; }

protected:
    std::unique_ptr<VideoFrameInfo> videoInfo_;
    std::unique_ptr<AudioFrameInfo> audioInfo_;
};

#endif // HALFWAY_MEDIA_MEDIA_SINK_H
