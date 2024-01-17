//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_SINK_H
#define HALFWAY_MEDIA_MEDIA_SINK_H

#include "../base/media_frame_pipeline.h"

class MediaSink : public FrameSink {
public:
    MediaSink() = default;
    ~MediaSink() override = default;

    virtual bool Init() = 0;

private:
};

#endif // HALFWAY_MEDIA_MEDIA_SINK_H
