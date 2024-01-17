//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_FILE_SINK_H
#define HALFWAY_MEDIA_MEDIA_FILE_SINK_H

#include "../base/media_sink.h"
#include <memory>

class MediaFileSink : public MediaSink {
public:
    ~MediaFileSink() override = default;

    template <typename... Args>
    static std::shared_ptr<MediaFileSink> Create(Args... args)
    {
        return std::shared_ptr<MediaFileSink>(new MediaFileSink(args...));
    }

    // impl FrameSink
    void OnFrame(const std::shared_ptr<Frame> &frame) override;

    // impl MediaSink
    bool Init() override;

private:
    MediaFileSink() = default;
};
#endif // HALFWAY_MEDIA_MEDIA_FILE_SINK_H