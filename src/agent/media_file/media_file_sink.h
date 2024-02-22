//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_FILE_SINK_H
#define HALFWAY_MEDIA_MEDIA_FILE_SINK_H

#include "../base/media_sink.h"
#include <memory>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class MediaFileSink : public MediaSink {
public:
    ~MediaFileSink() override ;

    template <typename... Args>
    static std::shared_ptr<MediaFileSink> Create(std::string file)
    {
        return std::shared_ptr<MediaFileSink>(new MediaFileSink(std::move(file)));
    }

    // impl FrameSink
    void OnFrame(const std::shared_ptr<Frame> &frame) override;

    // impl MediaSink
    bool Init() override;

private:
    MediaFileSink(std::string fileName) : fileName_(std::move(fileName)) {}

private:
    std::string fileName_;

    AVFormatContext *avFmtCtx_ = nullptr;
    AVStream *videoStream_ = nullptr;
    AVStream *audioStream_ = nullptr;
    AVPacket *avPacket_ = nullptr;
};
#endif // HALFWAY_MEDIA_MEDIA_FILE_SINK_H