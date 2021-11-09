//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_AUDIOENCODER_H
#define HALFWAYLIVE_AUDIOENCODER_H

#include "MediaFramePipeline.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
}

class AudioEncoder : public FrameSource, public FrameDestination {
  public:
    explicit AudioEncoder(FrameFormat format);
    virtual ~AudioEncoder();
    void onFrame(const Frame &frame) override;

    bool init();
    bool addAudioFrame(const Frame &audioFrame);
    void flush();

  protected:
    bool initEncoder(FrameFormat format);
    bool addToFifo(const Frame &audioFrame);
    void encode();
    void sendOut(AVPacket &pkt);

  private:
    FrameFormat _format;
    uint32_t _timestampOffset;
    bool _valid;

    int32_t _channels;
    int32_t _sampleRate;

    AVCodecContext *_audioEnc;
    AVAudioFifo *_audioFifo;
    AVFrame *_audioFrame;
};

#endif  // HALFWAYLIVE_AUDIOENCODER_H
