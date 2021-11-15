//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "Resampling.h"

#include "Utils.h"

Resampling::Resampling() : _swrCtx(nullptr), _inited(false), _dstData(nullptr), _swrSamplesCount(0), _dstLinesize(0) {}

Resampling::~Resampling()
{
    if (_swrCtx) {
        swr_free(&_swrCtx);
    }

    delete _dstData;
    _inited = false;
}

bool Resampling::F32ToS16(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples)
{
    if (frame->format != AV_SAMPLE_FMT_FLT) {
        return false;
    }

    if (!_inited) {
        init(AV_SAMPLE_FMT_FLT, frame->sample_rate, frame->channels, AV_SAMPLE_FMT_S16, frame->sample_rate,
             frame->channels);
        if (!_inited)
            return false;
    }

    // reallocate memory if there is not enough memory
    int dstNbSamples = (int)av_rescale_rnd(swr_get_delay(_swrCtx, frame->sample_rate) + frame->nb_samples,
                                           frame->sample_rate, frame->sample_rate, AV_ROUND_UP);
    if (dstNbSamples > _swrSamplesCount) {
        logger("reallocate audio swr samples buffer %d -> %d", _swrSamplesCount, dstNbSamples);
        av_freep(&_dstData[0]);
        int ret = av_samples_alloc(_dstData, &_dstLinesize, frame->channels, dstNbSamples, AV_SAMPLE_FMT_S16, 1);
        if (ret < 0) {
            logger("fail to reallocate swr samples, %s", ff_err2str(ret));
            return false;
        }
        _swrSamplesCount = dstNbSamples;
    }

    // convert pcm
    int ret = swr_convert(_swrCtx, _dstData, _dstLinesize, (const uint8_t **)&frame->data[0], frame->nb_samples);
    if (ret < 0) {
        logger("Error while converting, %s", ff_err2str(ret));
        return false;
    }

    int bufSize = av_samples_get_buffer_size(nullptr, frame->channels, ret, AV_SAMPLE_FMT_S16, 1);
    logger("get sample buffer size = %d", bufSize);

    *pOutData = _dstData[0];
    if (pOutNbSamples)
        *pOutNbSamples = ret;
    return true;
}

bool Resampling::init(enum AVSampleFormat inSampleFormat, int inSampleRate, int inChannels,
                      enum AVSampleFormat outSampleFormat, int outSampleRate, int outChannels)
{
    logger("init resampler %s-%d-%d -> %s-%d-%d", av_get_sample_fmt_name(inSampleFormat), inSampleRate, inChannels,
           av_get_sample_fmt_name(outSampleFormat), outSampleRate, outChannels);

    _swrCtx = swr_alloc_set_opts(nullptr, av_get_default_channel_layout(outChannels), outSampleFormat, outSampleRate,
                                 av_get_default_channel_layout(inChannels), inSampleFormat, inSampleRate, 0, nullptr);
    if (!_swrCtx) {
        logger("swr_alloc_set_opts error");
        return false;
    }

    int ret = swr_init(_swrCtx);
    if (ret < 0) {
        logger("fail to initialize the resampling context, %s", ff_err2str(ret));
        swr_free(&_swrCtx);
        return false;
    }

    // suppose src_nb_samples is 1024
    _swrSamplesCount = (int)av_rescale_rnd(1024, inSampleRate, outSampleRate, AV_ROUND_UP);
    ret =
        av_samples_alloc_array_and_samples(&_dstData, &_dstLinesize, outChannels, _swrSamplesCount, outSampleFormat, 1);
    if (ret < 0) {
        logger("Could not allocate destination samples\n");
        swr_free(&_swrCtx);
        return false;
    }

    _inited = true;
    return true;
}

// just for test
void Resampling::onFrame(const Frame &frame)
{
    logger("on frame size = %d", frame.length);
    DUMP_HEX(frame.payload, 10);

    AVFrame inFrame;
    inFrame.sample_rate = frame.additionalInfo.audio.sampleRate;
    inFrame.channels = frame.additionalInfo.audio.channels;
    inFrame.nb_samples = frame.additionalInfo.audio.nbSamples;
    inFrame.data[0] = frame.payload;
    inFrame.format = (frame.format == FRAME_FORMAT_PCM_48000_2_FLT) ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;

    Frame frame1 = frame;
    frame1.format = FRAME_FORMAT_PCM_48000_2_S16;

    uint8_t *data = nullptr;
    int nbSample = 0;
    bool ok = F32ToS16(&inFrame, &data, &nbSample);
    if (!ok)
        return;

    frame1.payload = data;
    frame1.length = av_samples_get_buffer_size(nullptr, inFrame.channels, nbSample, AV_SAMPLE_FMT_S16, 1);
    deliverFrame(frame1);
}
