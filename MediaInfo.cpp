//
// Created by lmshao on 2021/11/23.
//

#include "MediaInfo.h"

#include "Utils.h"

MediaInfo::MediaInfo() : _avFmtCtx(nullptr) {}

MediaInfo::~MediaInfo()
{
    avformat_close_input(&_avFmtCtx);
}

MediaInfo::MediaInfo(const std::string& file) : _avFmtCtx(nullptr)
{
    open(file);
}

bool MediaInfo::open(const std::string& filename)
{
    int ret = avformat_open_input(&_avFmtCtx, filename.c_str(), nullptr, nullptr);
    if (ret != 0) {
        logger("Failed to open input file '%s': %s", filename.c_str(), ff_err2str(ret));
        return false;
    }

    av_dump_format(_avFmtCtx, 1, filename.c_str(), 0);
    format = _avFmtCtx->iformat->name;
    duration = _avFmtCtx->duration / 1000000;

    int audioIndex, videoIndex;
    audioIndex = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    videoIndex = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (audioIndex >= 0) {
        AVStream* audio_st = _avFmtCtx->streams[audioIndex];
        logger("Has audio, audio stream number(%d), codec(%s), %d-%d", audioIndex,
               avcodec_get_name(audio_st->codecpar->codec_id), audio_st->codecpar->sample_rate,
               audio_st->codecpar->channels);

        _audio.format = avcodec_get_name(audio_st->codecpar->codec_id);
        _audio.sampleRate = audio_st->codecpar->sample_rate;
        _audio.channels = audio_st->codecpar->channels;
    }

    if (videoIndex >= 0) {
        AVStream* video_st = _avFmtCtx->streams[videoIndex];
        logger("Has video, video stream number(%d), codec(%s), %s, %dx%d", videoIndex,
               avcodec_get_name(video_st->codecpar->codec_id),
               avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile),
               video_st->codecpar->width, video_st->codecpar->height);

        _video.format = avcodec_get_name(video_st->codecpar->codec_id);
        _video.height = video_st->codecpar->height;
        _video.width = video_st->codecpar->width;
        _video.framerate = av_q2d(video_st->avg_frame_rate);
    }

    return true;
}
