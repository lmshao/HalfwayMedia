//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileIn.h"
extern "C" {
#include <libavutil/time.h>
}
#include <unistd.h>
#include <assert.h>

static inline int64_t timeRescale(uint32_t time, AVRational in, AVRational out)
{
    return av_rescale_q(time, in, out);
}

MediaFileIn::MediaFileIn(const std::string &filename) : MediaIn(filename), _running(false)
{
    _running = true;
    _timestampOffset = 0;
}

MediaFileIn::~MediaFileIn()
{
    logger("~");
};

bool MediaFileIn::open()
{
    int ret = avformat_open_input(&_avFmtCtx, _url.c_str(), nullptr, nullptr);
    if (ret != 0) {
        logger("Failed to open input file '%s': %s", _url.c_str(), ff_err2str(ret));
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
    av_dump_format(_avFmtCtx, 0, _url.c_str(), 0);

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
            _audioChannels = audio_st->codecpar->channels;
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
                logger("H264 profile: %s",
                       avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile));
                break;
            case AV_CODEC_ID_H265:
                _videoFormat = FRAME_FORMAT_H265;
                break;
            default:
                logger("Unsupported video codec");
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
    frame.timeStamp = 0;  // timeRescale(pkt->dts, _msTimeBase, _videoTimeBase);
    frame.additionalInfo.video.width = _videoWidth;
    frame.additionalInfo.video.height = _videoHeight;
    //    frame.additionalInfo.video.isKeyFrame = (pkt->flags & AV_PKT_FLAG_KEY);
    frame.additionalInfo.video.isKeyFrame = true;

    deliverFrame(frame);
    logger("deliver video frame, timestamp %ld(%ld), size %4d, %s %s",
           timeRescale(frame.timeStamp, _videoTimeBase, _msTimeBase), pkt->dts, frame.length,
           (pkt->flags & AV_PKT_FLAG_KEY) ? "key" : "non-key", getFormatStr(frame.format));
}

void MediaFileIn::deliverAudioFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _audioFormat;
    frame.payload = pkt->data;
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, _msTimeBase, _audioTimeBase);
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.sampleRate = _audioSampleRate;
    frame.additionalInfo.audio.channels = _audioChannels;
    frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels / 2;
    deliverFrame(frame);
    logger("deliver audio frame, timestamp %ld(%ld), size %4d",
           timeRescale(frame.timeStamp, _audioTimeBase, _msTimeBase), pkt->dts, frame.length);
}

void MediaFileIn::start()
{
    logger("");
    memset(&_avPacket, 0, sizeof(_avPacket));

    AVStream *video_st = _avFmtCtx->streams[_videoStreamIndex];
    AVRational time_base_q = { 1, AV_TIME_BASE };
    int64_t start_time = av_gettime();
    int64_t pts_time;
    int64_t now_time;

    av_init_packet(&_avPacket);
    _avPacket.data = nullptr;
    _avPacket.size = 0;

    AVPacket spsPacket, ppsPacket, tmpPacket;
    int cout = 0;

    //    FILE *fp;
    //    fp = fopen("1.h264", "wb");

    uint8_t startCode[4] = { 0x00, 0x00, 0x00, 0x01 };

    bool sendSpsPps = false;

    while (_running && av_read_frame(_avFmtCtx, &_avPacket) == 0) {
        logger("\n");
        if (_avPacket.stream_index == _videoStreamIndex) {  // pakcet is video
            logger("Get video frame packet, dts %ld, size %d", _avPacket.dts, _avPacket.size);

            if (!sendSpsPps) {
                int spsLength = 0;
                int ppsLength = 0;

                uint8_t *ex = _avFmtCtx->streams[_videoStreamIndex]->codecpar->extradata;
                spsLength = (ex[6] << 8) | ex[7];
                ppsLength = (ex[8 + spsLength + 1] << 8) | ex[8 + spsLength + 2];

                av_new_packet(&spsPacket, spsLength + 4);
                av_new_packet(&ppsPacket, ppsLength + 4);

                memcpy(spsPacket.data, startCode, 4);
                memcpy(spsPacket.data + 4, ex + 8, spsLength);
                deliverVideoFrame(&spsPacket);

                //                fwrite(startCode, 1, 4, fp);
                //                fwrite(ex + 8, 1, spsLength, fp);

                memcpy(ppsPacket.data, startCode, 4);
                memcpy(ppsPacket.data + 4, ex + 8 + spsLength + 2 + 1, ppsLength);
                deliverVideoFrame(&ppsPacket);

                //                fwrite(startCode, 1, 4, fp);
                //                fwrite(ex + 8 + spsLength + 2 + 1, 1, ppsLength, fp);
                cout = 2;
                sendSpsPps = true;
            }

            int nalLength = 0;
            uint8_t *data = _avPacket.data;
            while (data < _avPacket.data + _avPacket.size) {
                nalLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                if (nalLength > 0) {
                    memcpy(data, startCode, 4);
                    tmpPacket = _avPacket;
                    tmpPacket.data = data;
                    tmpPacket.size = nalLength + 4;
                    logger("send [%d], len = %d", cout++, tmpPacket.size);
                    dumpHex(data + 4, 10);
                    deliverVideoFrame(&tmpPacket);

                    //                    fwrite(startCode, 1, 4, fp);
                    //                    fwrite(data + 4, 1, nalLength, fp);
                }
                data = data + 4 + nalLength;
            }

            pts_time = av_rescale_q(_avPacket.pts, video_st->time_base, time_base_q);
            now_time = av_gettime() - start_time;
            if (pts_time - now_time > 0) {
                av_usleep(pts_time - now_time);
                logger("sleep %ld", (pts_time - now_time));
            }
        } else if (_avPacket.stream_index == _audioStreamIndex) {
            logger("audio");
            //            AVStream *audio_st = _avFmtCtx->streams[_audioStreamIndex];
            //            _avPacket.dts = timeRescale(_avPacket.dts, audio_st->time_base, _msTimeBase) +
            //            _timestampOffset; _avPacket.pts = timeRescale(_avPacket.pts, audio_st->time_base, _msTimeBase)
            //            + _timestampOffset; deliverAudioFrame(&_avPacket); logger("read audio _avPacket %s",
            //            dumpHex(_avPacket.data, 10));
            //
            //            AVRational time_base_q = { 1, AV_TIME_BASE };
            //            int64_t pts_time = av_rescale_q(_avPacket.pts, audio_st->time_base, time_base_q);
            //            int64_t now_time = av_gettime() - start_time;
            //            if (pts_time > now_time)
            //                av_usleep(pts_time - now_time);
            //
            //            logger("sleep %ld", (pts_time - now_time));
        }
        _lastTimstamp = _avPacket.dts;
        av_packet_unref(&_avPacket);
    }

    //    fclose(fp);
    sleep(10);
    logger("Thread exited!");
}
