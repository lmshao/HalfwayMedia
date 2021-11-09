//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_VIDEOENCODER_H
#define HALFWAYLIVE_VIDEOENCODER_H

#include "MediaFramePipeline.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

class VideoEncoder : public FrameSource, public FrameDestination {
  public:
    explicit VideoEncoder(FrameFormat format);
    ~VideoEncoder() override;

    void onFrame(const Frame &frame) override;
    int32_t generateStream(uint32_t width, uint32_t height, FrameDestination *dest);
    void degenerateStream(int32_t streamId);

  protected:
    bool initEncoder(FrameFormat format);
    void sendOut(AVPacket &pkt);
    void flushEncoder();

  private:
    bool _valid;
    int _width;
    int _height;
    int _frameRate;
    int _bitrateKbps;
    int _keyFrameIntervalSeconds;
    FrameFormat _format;
    AVCodecContext *_videoEnc{};
    AVFrame *_videoFrame{};
    std::shared_mutex _mutex;
};

#endif  // HALFWAYLIVE_VIDEOENCODER_H
