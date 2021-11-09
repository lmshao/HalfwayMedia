//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_VIDEODECODER_H
#define HALFWAYLIVE_VIDEODECODER_H

#include "MediaFramePipeline.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class VideoDecoder : public FrameSource, public FrameDestination {
  public:
    VideoDecoder();
    ~VideoDecoder();
    bool init(FrameFormat format);
    void onFrame(const Frame &frame) override;

  private:
    AVCodecContext *_decCtx;
    AVFrame *_decFrame;
    AVPacket *_packet;
    std::shared_ptr<uint8_t[]> _i420Buffer;
    int _i420BufferLength;
};

#endif  // HALFWAYLIVE_VIDEODECODER_H
