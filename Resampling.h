//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYMEDIA_RESAMPLING_H
#define HALFWAYMEDIA_RESAMPLING_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "MediaFramePipeline.h"

class Resampling : public FrameSource, public FrameDestination {
  public:
    Resampling();
    virtual ~Resampling();
    void onFrame(const Frame &frame) override;
    bool F32ToS16(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples);

  private:
    bool init(enum AVSampleFormat inSampleFormat, int inSampleRate, int inChannels, enum AVSampleFormat outSampleFormat,
              int outSampleRate, int outChannels);

    struct SwrContext *_swrCtx;

    uint8_t **_dstData;
    int _dstLinesize;
    int _swrSamplesCount;
    bool _inited;
};

#endif  // HALFWAYMEDIA_RESAMPLING_H
