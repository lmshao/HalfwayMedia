//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_SINK_H
#define HALFWAY_MEDIA_MEDIA_SINK_H

#include "../../common/frame.h"
#include "media_frame_pipeline.h"
#include <memory>

class MediaSink : public FrameSink {
public:
    MediaSink() = default;
    ~MediaSink() override = default;

    void SetMediaInfo(VideoFrameInfo *videoInfo, AudioFrameInfo *audioInfo);

    virtual bool Init() = 0;

protected:
    std::unique_ptr<VideoFrameInfo> videoInfo_;
    std::unique_ptr<AudioFrameInfo> audioInfo_;
};

#endif // HALFWAY_MEDIA_MEDIA_SINK_H
