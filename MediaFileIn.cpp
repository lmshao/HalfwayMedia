//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileIn.h"

static inline int64_t timeRescale(uint32_t time, AVRational in, AVRational out)
{
    return av_rescale_q(time, in, out);
}

MediaFileIn::MediaFileIn(const std::string &filename) : MediaIn(filename), _running(false)
{
    _running = true;
}

MediaFileIn::~MediaFileIn()
{
    logger("~");
};

bool MediaFileIn::open()
{
    int ret = avformat_open_input(&_avFmtCtx, _filename.c_str(), nullptr, nullptr);
    if (ret != 0) {
        logger("Failed to open input file '%s': %s", _filename.c_str(), ff_err2str(ret));
        return false;
    }

    logger("nb_stream=%d", _avFmtCtx->nb_streams);

    _avFmtCtx->fps_probe_size = 0;
    _avFmtCtx->max_ts_probe = 0;

    ret = avformat_find_stream_info(_avFmtCtx, nullptr);
    if (ret != 0) {
        logger("Failed to retrieve input stream information: %s", ff_err2str(ret));
        return false;
    }

    logger("Dump format");
    av_dump_format(_avFmtCtx, 0, _filename.c_str(), 0);

    return checkStream();
}

bool MediaFileIn::checkStream()
{
    if (!_avFmtCtx)
        return false;
#if 0
    for (int i = 0; i < _avFmtCtx->nb_streams; ++i) {
        AVCodecParameters *codecPar = _avFmtCtx->streams[i]->codecpar;
        switch (codecPar->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                if (_audioStreamIndex == -1)
                    _audioStreamIndex = i;
                break;
            case AVMEDIA_TYPE_VIDEO:
                if (_videoStreamIndex == -1)
                    _videoStreamIndex = i;
                break;
            default:
                break;
        }
    }
#else
    // find stream automatically
    int index;
    if ((index = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)) > -1) {
        _audioStreamIndex = index;
    }

    if ((index = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) > -1) {
        _videoStreamIndex = index;
    }

#endif
    logger("audio stream index %d", _audioStreamIndex);
    logger("video stream index %d", _videoStreamIndex);

    if (_audioStreamIndex >= 0) {
        AVStream *audio_st = _avFmtCtx->streams[_audioStreamIndex];
        logger("Has audio, audio stream number(%d), codec(%s), %d-%d", _audioStreamIndex,
               avcodec_get_name(audio_st->codecpar->codec_id), audio_st->codecpar->sample_rate,
               audio_st->codecpar->channels);
        switch (audio_st->codecpar->codec_id) {
            case AV_CODEC_ID_PCM_MULAW:
                _audioFormat = FRAME_FORMAT_PCMU;
                break;
            case AV_CODEC_ID_PCM_ALAW:
                _audioFormat = FRAME_FORMAT_PCMA;
                break;
            case AV_CODEC_ID_OPUS:
                _audioFormat = FRAME_FORMAT_OPUS;
                break;
            case AV_CODEC_ID_AAC:
                _audioFormat = FRAME_FORMAT_AAC;
                break;
            default:
                logger("Unsupported audio codec");
                break;
        }

        if (_audioFormat != FRAME_FORMAT_UNKNOWN) {
            _audioTimeBase.num = 1;
            _audioTimeBase.den = audio_st->codecpar->sample_rate;
            _audioSampleRate = audio_st->codecpar->sample_rate;
        }
    }

    if (_videoStreamIndex >= 0) {
        AVStream *video_st = _avFmtCtx->streams[_videoStreamIndex];
        logger("Has video, video stream number(%d), codec(%s), %s, %dx%d", _videoStreamIndex,
               avcodec_get_name(video_st->codecpar->codec_id),
               avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile),
               video_st->codecpar->width, video_st->codecpar->height);
        switch (video_st->codecpar->codec_id) {
            case AV_CODEC_ID_VP8:
                _videoFormat = FRAME_FORMAT_VP8;
                break;
            case AV_CODEC_ID_VP9:
                _videoFormat = FRAME_FORMAT_VP9;
                break;
            case AV_CODEC_ID_H264:
                _videoFormat = FRAME_FORMAT_H264;
                logger("H264 profile: %s", avcodec_profile_name(video_st->codecpar->codec_id,
                                                                video_st->codecpar->profile));
                break;
            case AV_CODEC_ID_H265:
                _videoFormat = FRAME_FORMAT_H265;
                break;
            default:
                logger("Unsupported audio codec");
                break;
        }

        if (_videoFormat != FRAME_FORMAT_UNKNOWN) {
            _videoWidth = video_st->codecpar->width;
            _videoHeight = video_st->codecpar->height;
            _videoTimeBase.num = 1;
            _videoTimeBase.den = 90000;
        }
    }

    _msTimeBase.num = 1;
    _msTimeBase.den = 1000;

    //    av_read_play(_avFmtCtx);

    return true;
}

void MediaFileIn::deliverVideoFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _videoFormat;
    frame.payload = pkt->data;
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, _msTimeBase, _videoTimeBase);
}

void MediaFileIn::deliverAudioFrame(AVPacket *pkt) {}

void MediaFileIn::receiveLoop() {}

void MediaFileIn::start() {}
