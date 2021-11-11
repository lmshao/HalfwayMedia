//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYMEDIA_IMAGECONVERSION_H
#define HALFWAYMEDIA_IMAGECONVERSION_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <stdint.h>

#include "MediaFramePipeline.h"

class ImageConversion : public FrameSource, public FrameDestination {
  public:
    ImageConversion();
    ~ImageConversion();

    bool BGRA32ToYUV420(uint8_t *bgra, uint8_t *yuv, int width, int length);

    void onFrame(const Frame &frame) override;

  private:
    bool init(AVPixelFormat srcFormat, int srcW, int srcH, AVPixelFormat dstFormat, int dstW, int dstH);

    AVFrame *_srcFrame;
    AVFrame *_dstFrame;
    SwsContext *_swsCtx;
    bool _inited;
};

#endif  // HALFWAYMEDIA_IMAGECONVERSION_H
