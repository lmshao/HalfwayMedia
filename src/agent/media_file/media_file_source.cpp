//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_file_source.h"
#include "../../common/data_buffer.h"
#include "../../common/utils.h"
#include "common/frame.h"
#include <memory>

extern "C" {
#include <libavutil/time.h>
}

MediaFileSource::~MediaFileSource()
{
    avformat_close_input(&avFmtCtx_);
    avFmtCtx_ = nullptr;
}

bool MediaFileSource::Init()
{
    LOGD("enter");

    AVDictionary *options = nullptr;
    int ret = avformat_open_input(&avFmtCtx_, fileName_.c_str(), nullptr, &options);
    if (ret != 0) {
        LOGE("Failed to open input file '%s': %s", fileName_.c_str(), ff_err2str(ret));
        return false;
    }

    LOGD("nb_stream=%d", avFmtCtx_->nb_streams);

    avFmtCtx_->fps_probe_size = 0;
    avFmtCtx_->max_ts_probe = 0;
    ret = avformat_find_stream_info(avFmtCtx_, nullptr);
    if (ret != 0) {
        LOGE("Failed to retrieve input stream information: %s", ff_err2str(ret));
        return false;
    }

    LOGD("Dump format");
    av_dump_format(avFmtCtx_, 0, fileName_.c_str(), 0);

    int index;
    if ((index = av_find_best_stream(avFmtCtx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)) > -1) {
        audioStreamIndex_ = index;
    }

    if ((index = av_find_best_stream(avFmtCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) > -1) {
        videoStreamIndex_ = index;
    }

    LOGD("videoStreamIndex: %d, audioStreamIndex: %d,", audioStreamIndex_, videoStreamIndex_);
    if (audioStreamIndex_ != -1) {
        AVStream *audioStream = avFmtCtx_->streams[audioStreamIndex_];
        LOGD("Has audio, audio stream number(%d), codec(%s), %d-%d", audioStreamIndex_,
             avcodec_get_name(audioStream->codecpar->codec_id), audioStream->codecpar->sample_rate,
             audioStream->codecpar->ch_layout.nb_channels);

        switch (audioStream->codecpar->codec_id) {
        case AV_CODEC_ID_PCM_MULAW:
            audioFmt_ = FRAME_FORMAT_PCMU;
            break;
        case AV_CODEC_ID_PCM_ALAW:
            audioFmt_ = FRAME_FORMAT_PCMA;
            break;
        case AV_CODEC_ID_AAC:
            audioFmt_ = FRAME_FORMAT_AAC;
            break;
        default:
            LOGE("Unsupported audio codec");
            break;
        }

        if (audioFmt_ != FRAME_FORMAT_UNKNOWN) {
            audioTimeBase_.num = 1;
            audioTimeBase_.den = audioStream->codecpar->sample_rate;
            audioInfo_.sampleRate = audioStream->codecpar->sample_rate;
            audioInfo_.channels = audioStream->codecpar->ch_layout.nb_channels;
        }
    }

    if (videoStreamIndex_ != -1) {
        AVStream *video_st = avFmtCtx_->streams[videoStreamIndex_];
        LOGD("Has video, video stream number(%d), codec(%s), %s, %dx%d", videoStreamIndex_,
             avcodec_get_name(video_st->codecpar->codec_id),
             avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile), video_st->codecpar->width,
             video_st->codecpar->height);

        switch (video_st->codecpar->codec_id) {
        case AV_CODEC_ID_VP8:
            videoFmt_ = FRAME_FORMAT_VP8;
            break;
        case AV_CODEC_ID_VP9:
            videoFmt_ = FRAME_FORMAT_VP9;
            break;
        case AV_CODEC_ID_H264: {
            videoFmt_ = FRAME_FORMAT_H264;
            LOGD("H264 profile: %s", avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile));

            uint8_t *ex = video_st->codecpar->extradata;
            int spsLength = (ex[6] << 8) | ex[7];
            int ppsLength = (ex[8 + spsLength + 1] << 8) | ex[8 + spsLength + 2];

            uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};
            spsFrame_ = std::make_shared<Frame>(spsLength + 4);
            spsFrame_->format = FRAME_FORMAT_H264;
            spsFrame_->Assign(startCode, 4);
            spsFrame_->Append(ex + 8, spsLength);

            ppsFrame_ = std::make_shared<Frame>(ppsLength + 4);
            ppsFrame_->format = FRAME_FORMAT_H264;
            ppsFrame_->Assign(startCode, 4);
            ppsFrame_->Append(ex + 8 + spsLength + 2 + 1, ppsLength);

            break;
        }
        case AV_CODEC_ID_H265:
            videoFmt_ = FRAME_FORMAT_H265;
            break;
        default:
            LOGE("Unsupported video codec");
            break;
        }

        if (videoFmt_ != FRAME_FORMAT_UNKNOWN) {
            videoInfo_.width = video_st->codecpar->width;
            videoInfo_.height = video_st->codecpar->height;
            videoTimeBase_.num = 1;
            videoTimeBase_.den = 90000;
        }
    }

    msTimeBase_.num = 1;
    msTimeBase_.den = 1000;

    return true;
}

void MediaFileSource::ReceiveDataLoop()
{
    LOGD("enter");
    AVStream *video_st = avFmtCtx_->streams[videoStreamIndex_];
    AVRational time_base_q = {1, AV_TIME_BASE};
    int64_t startTime = av_gettime();
    int64_t pts_time;
    int64_t now_time;
    LOGD(".");

    avPacket_ = av_packet_alloc();

    av_packet_unref(avPacket_);
    int cout = 1;
    uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};

    while (running_ && av_read_frame(avFmtCtx_, avPacket_) == 0) {
        if (avPacket_->stream_index == videoStreamIndex_) { // pakcet is video
            LOGD("Get video frame packet, dts %ld, size %d", avPacket_->dts, avPacket_->size);

            int nalLength = 0;
            uint8_t *data = avPacket_->data;
            while (data < avPacket_->data + avPacket_->size) {
                nalLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                if (nalLength > 0) {
                    bool isKeyFrame = false;
                    if ((data[4] & 0x1f) == 0x05) {
                        isKeyFrame = true;
                        LOGD("frame count: %d, len = %zu, SPS", cout++, spsFrame_->Size());
                        DeliverFrame(spsFrame_);

                        LOGD("frame count: %d, len = %zu, PPS", cout++, ppsFrame_->Size());
                        DeliverFrame(ppsFrame_);
                    }

                    auto frame = std::make_shared<Frame>(nalLength + 4);
                    frame->format = videoFmt_;
                    // frame->timestamp = 0;
                    frame->videoInfo.width = videoInfo_.width;
                    frame->videoInfo.height = videoInfo_.height;
                    frame->videoInfo.isKeyFrame = isKeyFrame;
                    frame->Assign(startCode, 4);
                    frame->Append(data + 4, nalLength);

                    LOGD("frame count: %d, len = %zu/%d, type: %02x", cout++, frame->Size(), avPacket_->size,
                         (data[4] & 0x1f));
                    DeliverFrame(frame);

                    data = data + 4 + nalLength;
                } else {
                    LOGE("check nalLength failed");
                }
            }

            pts_time = av_rescale_q(avPacket_->pts, video_st->time_base, time_base_q);
            now_time = av_gettime() - startTime;
            if (pts_time - now_time > 0) {
                LOGD("video sleep %ld", (pts_time - now_time));
                av_usleep(pts_time - now_time);
            }
        } else if (avPacket_->stream_index == audioStreamIndex_) {
            static int audioCount = 0;
            AVStream *audioStream = avFmtCtx_->streams[audioStreamIndex_];
            avPacket_->dts = av_rescale_q(avPacket_->dts, audioStream->time_base, msTimeBase_);
            avPacket_->pts = av_rescale_q(avPacket_->pts, audioStream->time_base, msTimeBase_);

            auto frame = std::make_shared<Frame>(avPacket_->size);
            frame->format = audioFmt_;
            // frame->timestamp = 0;
            frame->audioInfo.channels = audioInfo_.channels;
            frame->audioInfo.sampleRate = audioInfo_.sampleRate;
            frame->audioInfo.nbSamples = 1024; // aac
            frame->Assign(avPacket_->data, avPacket_->size);

            LOGD("audio frame count: %d, len = %zu", ++audioCount, frame->Size());
            DeliverFrame(frame);
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(avPacket_->pts, audioStream->time_base, time_base_q);
            int64_t now_time = av_gettime() - startTime;
            if (pts_time > now_time) {
                LOGD("audio sleep %ld", (pts_time - now_time));
                av_usleep(pts_time - now_time);
            }
        }

        av_packet_unref(avPacket_);
    }

    av_packet_free(&avPacket_);
    //    fclose(fp);
    LOGD("Thread exited!");
}