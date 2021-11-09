//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "AudioDecoder.h"

AudioDecoder::AudioDecoder()
    : _valid(false),
      _decCtx(nullptr),
      _decFrame(nullptr),
      _needResample(false),
      _swrCtx(nullptr),
      _swrSamplesData(nullptr),
      _swrSamplesLinesize(0),
      _swrSamplesCount(0),
      _swrInitialised(false),
      _outSampleFormat(AV_SAMPLE_FMT_S16),
      _outSampleRate(48000),
      _outChannels(2),
      _timestamp(0),
      _outFormat(FRAME_FORMAT_PCM_48000_2)
{
    logger("");
}
AudioDecoder::~AudioDecoder()
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_audioFifo) {
        av_audio_fifo_free(_audioFifo);
        _audioFifo = nullptr;
    }
    if (_audioFrame) {
        av_frame_free(&_audioFrame);
    }

    if (_packet) {
        av_packet_free(&_packet);
    }

    if (_swrCtx) {
        swr_free(&_swrCtx);
    }

    if (_swrSamplesData) {
        av_freep(&_swrSamplesData[0]);
        av_freep(&_swrSamplesData);
        _swrSamplesData = nullptr;
        _swrSamplesLinesize = 0;
    }
    _swrSamplesCount = 0;

    if (_decFrame) {
        av_frame_free(&_decFrame);
    }

    if (_decCtx) {
        avcodec_close(_decCtx);
        _decCtx = nullptr;
    }
}

bool AudioDecoder::init(FrameFormat format)
{
    // TODO:sth
    _valid = true;
    return true;
}

void AudioDecoder::onFrame(const Frame &frame)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_valid) {
        logger("Not valid");
        return;
    }

    if (!_decCtx) {
        if (!initDecoder(frame.format, frame.additionalInfo.audio.sampleRate, frame.additionalInfo.audio.channels)) {
            _valid = false;
            return;
        }
        if (!initResampler(_inSampleFormat, _inSampleRate, _inChannels, _outSampleFormat, _outSampleRate,
                           _outChannels)) {
            _valid = false;
            return;
        }
        if (!initFifo(_outSampleFormat, _outSampleRate, _outChannels)) {
            _valid = false;
            return;
        }
    }

    av_packet_unref(_packet);
    _packet->data = frame.payload;
    _packet->size = frame.length;

    int ret;
    ret = avcodec_send_packet(_decCtx, _packet);
    if (ret < 0) {
        logger("Error while send packet, %s", ff_err2str(ret));
        return;
    }

    ret = avcodec_receive_frame(_decCtx, _decFrame);
    if (ret == AVERROR(EAGAIN)) {
        logger("Retry receive frame, %s", ff_err2str(ret));
        return;
    } else if (ret < 0) {
        logger("Error while receive frame, %s", ff_err2str(ret));
        return;
    }

    if (!addFrameToFifo(_decFrame)) {
        logger("Error add frame to fifo");
        return;
    }

    while (av_audio_fifo_size(_audioFifo) >= _audioFrame->nb_samples) {
        int32_t n;

        n = av_audio_fifo_read(_audioFifo, reinterpret_cast<void **>(_audioFrame->data), _audioFrame->nb_samples);
        if (n != _audioFrame->nb_samples) {
            logger("Cannot read enough data from fifo, needed %d, read %d", _audioFrame->nb_samples, n);
            return;
        }

        if (_outFormat == FRAME_FORMAT_PCM_48000_2) {
            Frame outFrame;
            memset(&outFrame, 0, sizeof(outFrame));
            outFrame.format = FRAME_FORMAT_PCM_48000_2;
            outFrame.payload = reinterpret_cast<uint8_t *>(_audioFrame->data[0]);
            outFrame.length = _audioFrame->nb_samples * _outChannels * 2;
            outFrame.additionalInfo.audio.isRtpPacket = 0;
            outFrame.additionalInfo.audio.sampleRate = _outSampleRate;
            outFrame.additionalInfo.audio.channels = _outChannels;
            outFrame.additionalInfo.audio.nbSamples = _audioFrame->nb_samples;
            outFrame.timeStamp = _timestamp * outFrame.additionalInfo.audio.sampleRate / 1000;

            logger("deliverFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%d), %s",
                   getFormatStr(outFrame.format), outFrame.additionalInfo.audio.sampleRate,
                   outFrame.additionalInfo.audio.channels,
                   outFrame.timeStamp * 1000 / outFrame.additionalInfo.audio.sampleRate, outFrame.length,
                   outFrame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket");

            DUMP_HEX(outFrame.payload, 10);
            deliverFrame(outFrame);
        } else {
            logger("_outFormat != FRAME_FORMAT_PCM_48000_2");
        }
        _timestamp += 1000 * _audioFrame->nb_samples / _outSampleRate;
    }
}
bool AudioDecoder::initDecoder(FrameFormat format, uint32_t sampleRate, uint32_t channels)
{
    int ret;
    AVCodecID codecId;
    AVCodec *dec;

    switch (format) {
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
            codecId = AV_CODEC_ID_AAC;
            break;
        case FRAME_FORMAT_MP3:
            codecId = AV_CODEC_ID_MP3;
            break;
        default:
            logger("Invalid format(%s)", getFormatStr(format));
            return false;
    }

    logger("Created %s decoder.", getFormatStr(format));

    dec = avcodec_find_decoder(codecId);
    if (!dec) {
        logger("Could not find ffmpeg decoder %s", avcodec_get_name(codecId));
        return false;
    }

    _decCtx = avcodec_alloc_context3(dec);
    if (!_decCtx) {
        logger("Could not alloc ffmpeg decoder context");
        return false;
    }

    _decCtx->sample_fmt = AV_SAMPLE_FMT_FLT;
    _decCtx->sample_rate = sampleRate;
    _decCtx->channels = channels;
    _decCtx->channel_layout = av_get_default_channel_layout(channels);
    ret = avcodec_open2(_decCtx, dec, nullptr);
    if (ret < 0) {
        logger("Could not open ffmpeg decoder context, %s", ff_err2str(ret));
        return false;
    }

    _decFrame = av_frame_alloc();
    if (!_decFrame) {
        logger("Could not allocate dec frame");
        return false;
    }

    _inSampleFormat = _decCtx->sample_fmt;
    _inSampleRate = _decCtx->sample_rate;
    _inChannels = _decCtx->channels;

    _packet = av_packet_alloc();
    if (!_packet) {
        logger("Could not allocate av packet");
        return false;
    }

    return true;
}
bool AudioDecoder::initResampler(enum AVSampleFormat inSampleFormat, int inSampleRate, int inChannels,
                                 enum AVSampleFormat outSampleFormat, int outSampleRate, int outChannels)
{
    int ret;
    if (inSampleFormat == outSampleFormat && inSampleRate == outSampleRate && inChannels == outChannels) {
        _needResample = false;
        return true;
    }

    _needResample = true;

    logger("Init resampler %s-%d-%d -> %s-%d-%d", av_get_sample_fmt_name(inSampleFormat), inSampleRate, inChannels,
           av_get_sample_fmt_name(outSampleFormat), outSampleRate, outChannels);

    //    _swrCtx = swr_alloc();
    //
    //    /* set options */
    //    av_opt_set_sample_fmt(_swrCtx, "in_sample_fmt",      inSampleFormat,       0);
    //    av_opt_set_int       (_swrCtx, "in_sample_rate",     inSampleRate,         0);
    //    av_opt_set_int       (_swrCtx, "in_channel_count",   inChannels,           0);
    //    av_opt_set_sample_fmt(_swrCtx, "out_sample_fmt",     outSampleFormat,    0);
    //    av_opt_set_int       (_swrCtx, "out_sample_rate",    outSampleRate,      0);
    //    av_opt_set_int       (_swrCtx, "out_channel_count",  outChannels,        0);

    _swrCtx = swr_alloc_set_opts(nullptr, av_get_default_channel_layout(outChannels), outSampleFormat, outSampleRate,
                                 av_get_default_channel_layout(inChannels), inSampleFormat, inSampleRate, 0, nullptr);

    ret = swr_init(_swrCtx);
    if (ret < 0) {
        logger("Fail to initialize the resampling context, %s", ff_err2str(ret));
        swr_free(&_swrCtx);
        _swrCtx = nullptr;
        return false;
    }

    _swrSamplesCount = 2048;
    ret = av_samples_alloc_array_and_samples(&_swrSamplesData, &_swrSamplesLinesize, outChannels, _swrSamplesCount,
                                             outSampleFormat, 0);
    if (ret < 0) {
        logger("Could not allocate swr samples data, %s", ff_err2str(ret));
        swr_free(&_swrCtx);
        _swrCtx = nullptr;
        return false;
    }

    return true;
}

bool AudioDecoder::initFifo(enum AVSampleFormat sampleFmt, uint32_t sampleRate, uint32_t channels)
{
    int ret;

    _audioFifo = av_audio_fifo_alloc(sampleFmt, channels, 1);
    if (!_audioFifo) {
        logger("Cannot allocate audio fifo");
        return false;
    }

    _audioFrame = av_frame_alloc();
    if (!_audioFrame) {
        logger("Cannot allocate audio frame");
        return false;
    }

    _audioFrame->nb_samples = (int)sampleRate / 100;  // 10ms
    _audioFrame->format = sampleFmt;
    _audioFrame->channel_layout = av_get_default_channel_layout(channels);
    _audioFrame->sample_rate = sampleRate;

    ret = av_frame_get_buffer(_audioFrame, 0);
    if (ret < 0) {
        logger("Cannot get audio frame buffer, %s", ff_err2str(ret));
        return false;
    }

    return true;
}

bool AudioDecoder::resampleFrame(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples)
{
    int ret;
    int dstNbSamples;

    if (!_swrCtx) return false;

    // compute destination number of samples
    dstNbSamples = av_rescale_rnd(swr_get_delay(_swrCtx, _inSampleRate) + frame->nb_samples, _outSampleRate,
                                  _inSampleRate, AV_ROUND_UP);

    if (dstNbSamples > _swrSamplesCount) {
        int newSize = 2 * dstNbSamples;

        logger("Realloc audio swr samples buffer %d -> %d", _swrSamplesCount, newSize);

        av_freep(&_swrSamplesData[0]);
        ret = av_samples_alloc(_swrSamplesData, &_swrSamplesLinesize, _outChannels, newSize, _outSampleFormat, 1);
        if (ret < 0) {
            logger("Fail to realloc swr samples, %s", ff_err2str(ret));
            return false;
        }
        _swrSamplesCount = newSize;
    }

    /* convert to destination format */
    ret = swr_convert(_swrCtx, _swrSamplesData, dstNbSamples, (const uint8_t **)frame->data, frame->nb_samples);
    if (ret < 0) {
        logger("Error while converting, %s", ff_err2str(ret));
        return false;
    }

    *pOutData = _swrSamplesData[0];
    *pOutNbSamples = ret;
    return true;
}

bool AudioDecoder::addFrameToFifo(AVFrame *frame)
{
    uint8_t *data;
    int samplesPerChannel;

    if (_needResample) {
        if (!resampleFrame(frame, &data, &samplesPerChannel)) return false;
    } else {
        data = (uint8_t *)frame->data;
        samplesPerChannel = frame->nb_samples;
    }

    int32_t n;
    n = av_audio_fifo_write(_audioFifo, reinterpret_cast<void **>(&data), samplesPerChannel);
    if (n < samplesPerChannel) {
        logger("Cannot not write data to fifo, bnSamples %d, writed %d", samplesPerChannel, n);
        return false;
    }

    return true;
}
