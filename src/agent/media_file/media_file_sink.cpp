#include "media_file_sink.h"
#include "common/log.h"
#include "common/utils.h"
#include <cstdint>
#include <netinet/in.h>

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/time.h>
}

MediaFileSink::~MediaFileSink()
{
    Stop();
}

void MediaFileSink::OnFrame(const std::shared_ptr<Frame> &frame)
{
    if (!avFmtCtx_) {
        LOGE("avFmtCtx is nullptr");
        exit(0);
        return;
    }

    if (!avPacket_) {
        avPacket_ = av_packet_alloc();
    }

    av_packet_unref(avPacket_);

    if (frame->format == FRAME_FORMAT_H264) {
        int nalLength = frame->Size() - 4;
        LOGW("NALU size: %d", nalLength);
        *(uint32_t *)frame->Data() = htonl(nalLength);

        avPacket_->stream_index = videoStream_->index;
        avPacket_->data = frame->Data();
        avPacket_->size = frame->Size();

        LOGW("Video timestamp : %d", (int)(frame->timestamp / 90)); // ms /1000 * 90000 = rtp_ts

        // avPacket_->pts = avPacket_->dts = frame->timestamp;
        avPacket_->pts = avPacket_->dts = (av_gettime_relative() / 1000'000);

        avPacket_->duration = 90000 / 25;
        avPacket_->flags = frame->videoInfo.isKeyFrame ? AV_PKT_FLAG_KEY : 0;
        avPacket_->pos = -1;
    } else if (frame->format == FRAME_FORMAT_AAC) {
        avPacket_->stream_index = audioStream_->index;
        avPacket_->data = frame->Data() + 7;
        avPacket_->size = frame->Size() - 7;
        LOGE("Audio timestamp : %d", (int)(frame->timestamp / 48));
        // avPacket_->pts = avPacket_->dts = frame->timestamp;
        avPacket_->pts = avPacket_->dts = (av_gettime_relative() / 1000'000);

        avPacket_->duration = 1024;
        avPacket_->flags = 0;
        avPacket_->pos = -1;
    } else {
        LOGW("Unsupport frame format");
    }

    // if (av_interleaved_write_frame(avFmtCtx_, avPacket_) != 0) {
    //     LOGE("av_interleaved_write_frame failed");
    // }
}

bool MediaFileSink::Init()
{
    if (fileName_.empty()) {
        return false;
    }

    if (!videoInfo_ && !audioInfo_) {
        LOGE("u need to call SetMediaInfo() first");
        return false;
    }

    std::string suffix;
    auto i = fileName_.find_last_of('.');
    if (i != std::string::npos) {
        suffix = fileName_.substr(i + 1);
        if (suffix != "mp4" && suffix != "mkv" && suffix != "ts" && suffix != "flv") {
            fileName_ = fileName_.substr(0, i) + ".mp4";
        }
    } else {
        fileName_ = fileName_ + ".mp4";
    }

    int ret = avformat_alloc_output_context2(&avFmtCtx_, nullptr, nullptr, fileName_.c_str());
    if (ret < 0) {
        LOGE("Failed to alloc output ctx: %s", ff_strerror(ret));
        return false;
    }

    if (videoInfo_) {
        videoStream_ = avformat_new_stream(avFmtCtx_, nullptr);
        if (!videoStream_) {
            LOGE("Failed to new stream");
            return false;
        }

        LOGD("width: %d, height: %d", videoInfo_->width, videoInfo_->height);
        videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        videoStream_->codecpar->codec_id = AV_CODEC_ID_H264;
        videoStream_->codecpar->width = videoInfo_->width;
        videoStream_->codecpar->height = videoInfo_->height;
        videoStream_->codecpar->format = AV_PIX_FMT_YUV420P;
        // videoStream_->codecpar->bit_rate = 1000000;
        videoStream_->time_base = (AVRational){1, 90000};
    }

    if (audioInfo_) {
        audioStream_ = avformat_new_stream(avFmtCtx_, nullptr);
        if (!audioStream_) {
            LOGE("Failed to new stream");
            return false;
        }

        LOGD("sampleRate: %d, channels: %d", audioInfo_->sampleRate, audioInfo_->channels);
        audioStream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        audioStream_->codecpar->codec_id = AV_CODEC_ID_AAC;
        audioStream_->codecpar->sample_rate = audioInfo_->sampleRate;
        audioStream_->codecpar->ch_layout.nb_channels = audioInfo_->channels;
        audioStream_->codecpar->format = AV_SAMPLE_FMT_FLTP;
        // audioStream_->codecpar->bit_rate = 128000;
        audioStream_->time_base = (AVRational){1, (int)audioInfo_->sampleRate};
    }

    if (!(avFmtCtx_->flags & AVFMT_NOFILE)) {
        if (avio_open(&avFmtCtx_->pb, fileName_.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGE("Failed to open output file '%s'", fileName_.c_str());
            return false;
        }
    }

    ret = avformat_write_header(avFmtCtx_, nullptr);
    if (ret < 0) {
        LOGE("Failed to write header: %s", ff_strerror(ret));
        return false;
    }

    avPacket_ = av_packet_alloc();
    if (!avPacket_) {
        LOGE("Failed to alloc av packet");
        return false;
    }

    return true;
}

bool MediaFileSink::Stop()
{
    if (avFmtCtx_) {
        av_write_trailer(avFmtCtx_);
        avformat_close_input(&avFmtCtx_);
    }

    if (avPacket_) {
        av_packet_free(&avPacket_);
    }

    return true;
}