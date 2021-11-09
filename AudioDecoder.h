//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_AUDIODECODER_H
#define HALFWAYLIVE_AUDIODECODER_H

#include "MediaFramePipeline.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

class AudioDecoder : public FrameSource, public FrameDestination {
  public:
    AudioDecoder();
    ~AudioDecoder() override;

    bool init(FrameFormat format);
    void onFrame(const Frame &frame) override;

  protected:
    bool initDecoder(FrameFormat format, uint32_t sampleRate, uint32_t channels);
    bool initResampler(enum AVSampleFormat inFormat, int inSampleRate, int inChannels,
                       enum AVSampleFormat outSampleFormat, int outSampleRate, int outChannels);
    bool initFifo(enum AVSampleFormat sampleFmt, uint32_t sampleRate, uint32_t channels);

    bool resampleFrame(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples);
    bool addFrameToFifo(AVFrame *frame);

  private:
    std::mutex _mutex;
    bool _valid;

    AVCodecContext *_decCtx;
    AVFrame *_decFrame;
    AVPacket *_packet;

    bool _needResample;
    struct SwrContext *_swrCtx;
    uint8_t **_swrSamplesData;
    int _swrSamplesLinesize;
    int _swrSamplesCount;
    bool _swrInitialised;

    AVAudioFifo *_audioFifo;
    AVFrame *_audioFrame;

    enum AVSampleFormat _inSampleFormat;
    int _inSampleRate;
    int _inChannels;

    enum AVSampleFormat _outSampleFormat;
    int _outSampleRate;
    int _outChannels;

    int64_t _timestamp;
    FrameFormat _outFormat;
};

#endif  // HALFWAYLIVE_AUDIODECODER_H
