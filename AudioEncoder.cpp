//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "AudioEncoder.h"

#include <sys/time.h>

#include "AudioTime.h"

static enum AVSampleFormat getCodecPreferedSampleFmt(AVCodec *codec, enum AVSampleFormat PreferedSampleFmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == PreferedSampleFmt) {
            return PreferedSampleFmt;
        }
        p++;
    }
    return codec->sample_fmts[0];
}

AudioEncoder::AudioEncoder(FrameFormat format)
    : _format(format),
      _timestampOffset(0),
      _valid(false),
      _channels(2),
      _sampleRate(48000),
      _audioEnc(nullptr),
      _audioFifo(nullptr),
      _audioFrame(nullptr),
      _audioPkt(nullptr),
      _resampler(nullptr)
{
}

AudioEncoder::~AudioEncoder()
{
    if (!_valid)
        return;

    if (_audioFrame) {
        av_frame_free(&_audioFrame);
    }

    if (_audioPkt) {
        av_packet_free(&_audioPkt);
    }

    if (_audioFifo) {
        av_audio_fifo_free(_audioFifo);
        _audioFifo = nullptr;
    }

    if (_audioEnc) {
        avcodec_close(_audioEnc);
        _audioEnc = nullptr;
    }

    _valid = false;
    _format = FRAME_FORMAT_UNKNOWN;
}

void AudioEncoder::onFrame(const Frame &frame)
{
    if (!_valid)
        return;

    if (frame.format != FRAME_FORMAT_PCM_48000_2_FLT && frame.format != FRAME_FORMAT_PCM_48000_2_S16) {
        logger("Unsupported format %s", getFormatStr(frame.format));
        return;
    }

    if (frame.format == FRAME_FORMAT_PCM_48000_2_FLT) {
        // Resampling
        if (!_resampler) {
            _resampler = new Resampling();
        }

        AVFrame inFrame;
        inFrame.sample_rate = frame.additionalInfo.audio.sampleRate;
        inFrame.channels = frame.additionalInfo.audio.channels;
        inFrame.nb_samples = frame.additionalInfo.audio.nbSamples;
        inFrame.data[0] = frame.payload;
        inFrame.format = (frame.format == FRAME_FORMAT_PCM_48000_2_FLT) ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;

        Frame frame1 = frame;
        frame1.format = FRAME_FORMAT_PCM_48000_2_S16;

        int nbSample = 0;
        if (!_resampler->F32ToS16(&inFrame, &frame1.payload, &nbSample))
            return;

        frame1.length = av_samples_get_buffer_size(nullptr, inFrame.channels, nbSample, AV_SAMPLE_FMT_S16, 1);

        if (!addToFifo(frame1))
            return;
    } else {
        if (!addToFifo(frame))
            return;
    }

    encode();
}

bool AudioEncoder::init()
{
    if (!initEncoder(_format)) {
        return false;
    }

    _valid = true;
    return true;
}

void AudioEncoder::flush()
{
    while (_valid) {
        avcodec_send_frame(_audioEnc, nullptr);
        int ret = avcodec_receive_packet(_audioEnc, _audioPkt);
        if (ret == 0) {
            logger("get pkg in encoder");
            sendOut(*_audioPkt);
            av_packet_unref(_audioPkt);
        } else if (ret == AVERROR_EOF) {
            logger("complete encoding\n");
            break;
        } else {
            logger("Error encoding frame\n");
            break;
        }
    }
    logger("flush audio encoder over");
}

bool AudioEncoder::initEncoder(const FrameFormat format)
{
    int ret;
    AVCodec *codec;

    switch (format) {
        case FRAME_FORMAT_AAC_48000_2:
            codec = avcodec_find_encoder_by_name("libfdk_aac");
            if (!codec) {
                logger("Can not find audio encoder ffmpeg/libfdk_aac, use aac instead");
                codec = avcodec_find_encoder_by_name("aac");
                if (!codec) {
                    logger("Can not find audio encoder ffmpeg/aac");
                    return false;
                }
            }
            break;

        case FRAME_FORMAT_OPUS:
            codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
            if (!codec) {
                logger("Could not find audio encoder %s", avcodec_get_name(AV_CODEC_ID_OPUS));
                return false;
            }
            break;

        default:
            logger("Encoder %s is not supported", getFormatStr(format));
            return false;
    }

    // context
    _audioEnc = avcodec_alloc_context3(codec);
    if (!_audioEnc) {
        logger("Can not alloc avcodec context");
        return false;
    }

    _audioEnc->bit_rate = 128000;
    _audioEnc->channels = _channels;
    _audioEnc->channel_layout = av_get_default_channel_layout(_audioEnc->channels);
    _audioEnc->sample_rate = _sampleRate;
    // AV_SAMPLE_FMT_S16 | AV_SAMPLE_FMT_FLT
    _audioEnc->sample_fmt = AV_SAMPLE_FMT_S16;  // getCodecPreferedSampleFmt(codec, AV_SAMPLE_FMT_FLTP);
    _audioEnc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(_audioEnc, codec, nullptr);
    if (ret < 0) {
        logger("Cannot open output audio codec, %s", ff_err2str(ret));
        goto fail;
    }

    // fifo
    if (_audioFifo) {
        av_audio_fifo_free(_audioFifo);
        _audioFifo = nullptr;
    }

    _audioFifo = av_audio_fifo_alloc(_audioEnc->sample_fmt, _audioEnc->channels, 1);  // default:1
    if (!_audioFifo) {
        logger("Cannot allocate audio fifo");
        goto fail;
    }

    // frame
    if (_audioFrame) {
        av_frame_free(&_audioFrame);
    }

    _audioFrame = av_frame_alloc();
    if (!_audioFrame) {
        logger("Cannot allocate audio frame");
        goto fail;
    }

    _audioFrame->nb_samples = _audioEnc->frame_size;
    _audioFrame->format = _audioEnc->sample_fmt;
    _audioFrame->channel_layout = _audioEnc->channel_layout;
    _audioFrame->sample_rate = _audioEnc->sample_rate;

    ret = av_frame_get_buffer(_audioFrame, 0);
    if (ret < 0) {
        logger("Cannot get audio frame buffer, %s", ff_err2str(ret));
        goto fail;
    }

    if (_audioPkt) {
        av_packet_free(&_audioPkt);
    }

    _audioPkt = av_packet_alloc();
    if (!_audioPkt) {
        logger("Cannot allocate audio packet");
        goto fail;
    }

    logger("Audio encoder frame_size %d, sample_rate %d, channels %d", _audioEnc->frame_size, _audioEnc->sample_rate,
           _audioEnc->channels);

    return true;

fail:
    if (_audioPkt) {
        av_packet_free(&_audioPkt);
    }

    if (_audioFrame) {
        av_frame_free(&_audioFrame);
    }

    if (_audioFifo) {
        av_audio_fifo_free(_audioFifo);
        _audioFifo = nullptr;
    }

    avcodec_free_context(&_audioEnc);
    _audioEnc = nullptr;

    return false;
}

bool AudioEncoder::addToFifo(const Frame &audioFrame)
{
    if (audioFrame.additionalInfo.audio.sampleRate != (uint32_t)_sampleRate ||
        (int32_t)audioFrame.additionalInfo.audio.channels != _channels) {
        logger("Invalid audio frame, %s(%d-%ld), want(%d-%d)", getFormatStr(_format),
               audioFrame.additionalInfo.audio.sampleRate, audioFrame.additionalInfo.audio.channels, _sampleRate,
               _channels);
        return false;
    }

    std::shared_ptr<uint8_t[]> cache(new uint8_t[audioFrame.length]);
    memcpy(cache.get(), audioFrame.payload, audioFrame.length);

    DUMP_HEX(audioFrame.payload, 10);
    logger("%d", av_audio_fifo_space(_audioFifo));
    //    void *data = (void*)audioFrame.payload;
    void *data = cache.get();

    uint32_t n =
        av_audio_fifo_write(_audioFifo, reinterpret_cast<void **>(&data), audioFrame.additionalInfo.audio.nbSamples);
    if (n < audioFrame.additionalInfo.audio.nbSamples) {
        logger("Cannot not write data to fifo, bnSamples %ld, writed %d", audioFrame.additionalInfo.audio.nbSamples, n);
        return false;
    }

    return true;
}

void AudioEncoder::encode()
{
    logger("encoder");
    int ret = 0, n = 0, fifo_size = 0;

    while (true) {
        fifo_size = av_audio_fifo_size(_audioFifo);
        if (fifo_size < _audioEnc->frame_size) {
            break;
        }

        n = av_audio_fifo_read(_audioFifo, (void **)(_audioFrame->data), _audioEnc->frame_size);
        if (n != _audioEnc->frame_size) {
            logger("Cannot read enough data from fifo, needed %d, read %d", _audioEnc->frame_size, n);
            return;
        }

        ret = avcodec_send_frame(_audioEnc, _audioFrame);
        if (ret < 0) {
            logger("avcodec_send_frame, %s", ff_err2str(ret));
            return;
        }

        ret = avcodec_receive_packet(_audioEnc, _audioPkt);
        if (ret < 0) {
            logger("avcodec_receive_packet, %s", ff_err2str(ret));
            return;
        }

        logger("after encoding ffmpeg pts: %ld\n", _audioPkt->dts);

        sendOut(*_audioPkt);
        av_packet_unref(_audioPkt);
    }
}

void AudioEncoder::sendOut(AVPacket &pkt)
{
    logger("");
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _format;
    frame.payload = pkt.data;
    frame.length = pkt.size;
    frame.additionalInfo.audio.nbSamples = _audioEnc->frame_size;
    frame.additionalInfo.audio.sampleRate = _audioEnc->sample_rate;
    frame.additionalInfo.audio.channels = _audioEnc->channels;
    frame.timeStamp = AudioTime::currentTime(); /* frame.additionalInfo.audio.sampleRate / 1000;*/

    logger("deliverFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%d), %s", getFormatStr(frame.format),
           frame.additionalInfo.audio.sampleRate, frame.additionalInfo.audio.channels,
           frame.timeStamp * 1000 / frame.additionalInfo.audio.sampleRate, frame.length,
           frame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket");

    deliverFrame(frame);
}

bool AudioEncoder::resampling(const Frame &frame)
{
    return false;
}
