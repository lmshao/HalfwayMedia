//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_FILE_SOURCE_H
#define HALFWAY_MEDIA_MEDIA_FILE_SOURCE_H

#include "../base/media_source.h"
#include "common/frame.h"
#include <memory>
#include <string>
#include <utility>

extern "C" {
#include <libavformat/avformat.h>
}

class MediaFileSource : public MediaSource {
public:
    ~MediaFileSource() override;

    template <typename... Args>
    static std::shared_ptr<MediaFileSource> Create(Args... args)
    {
        return std::shared_ptr<MediaFileSource>(new MediaFileSource(args...));
    }

    // impl MediaSource
    bool Init() override;

private:
    MediaFileSource() = default;
    explicit MediaFileSource(std::string fileName) : fileName_(std::move(fileName)) {}

    // impl MediaSource
    void ReceiveDataLoop() override;

private:
    std::string fileName_;

    AVFormatContext *avFmtCtx_ = nullptr;
    AVPacket *avPacket_ = nullptr;

    AVRational msTimeBase_;
    AVRational audioTimeBase_;
    AVRational videoTimeBase_;

    int audioStreamIndex_ = -1;
    int videoStreamIndex_ = -1;

    FrameFormat audioFmt_ = FRAME_FORMAT_UNKNOWN;
    FrameFormat videoFmt_ = FRAME_FORMAT_UNKNOWN;
    VideoFrameInfo videoInfo_{};
    AudioFrameInfo audioInfo_{};

    std::shared_ptr<Frame> spsFrame_;
    std::shared_ptr<Frame> ppsFrame_;
};

#endif // HALFWAY_MEDIA_MEDIA_FILE_SOURCE_H