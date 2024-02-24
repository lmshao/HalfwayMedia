#include "media_file_sink.h"
#include "common/log.h"
#include "common/utils.h"

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
        return;
    }

    if (!avPacket_) {
        avPacket_ = av_packet_alloc();
    }

    av_packet_unref(avPacket_);

    if (frame->format == FRAME_FORMAT_H264) {
        avPacket_->stream_index = videoStream_->index;
        avPacket_->data = frame->Data();
        avPacket_->size = frame->Size();
        avPacket_->pts = avPacket_->dts = frame->timestamp; // TODO: fix this av_gettime() / 1000000
        avPacket_->duration = 90000 / 25;
        avPacket_->flags = frame->videoInfo.isKeyFrame ? AV_PKT_FLAG_KEY : 0;
        avPacket_->pos = -1;
    } else if (frame->format == FRAME_FORMAT_AAC) {
        avPacket_->stream_index = audioStream_->index;
        avPacket_->data = frame->Data();
        avPacket_->size = frame->Size();
        avPacket_->pts = avPacket_->dts = frame->timestamp; // TODO: fix this
        avPacket_->duration = 1024;
        avPacket_->flags = 0;
        avPacket_->pos = -1;
    } else {
        LOGW("Unsupport frame format");
    }

    av_interleaved_write_frame(avFmtCtx_, avPacket_);
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

        videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        videoStream_->codecpar->codec_id = AV_CODEC_ID_H264;
        videoStream_->codecpar->width = 1280;
        videoStream_->codecpar->height = 720;
        videoStream_->codecpar->format = AV_PIX_FMT_YUV420P;
        videoStream_->codecpar->bit_rate = 1000000;
    }

    if (audioInfo_) {
        audioStream_ = avformat_new_stream(avFmtCtx_, nullptr);
        if (!audioStream_) {
            LOGE("Failed to new stream");
            return false;
        }

        audioStream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        audioStream_->codecpar->codec_id = AV_CODEC_ID_AAC;
        audioStream_->codecpar->sample_rate = 44100;
        audioStream_->codecpar->ch_layout.nb_channels = 2;
        audioStream_->codecpar->format = AV_SAMPLE_FMT_FLTP;
        audioStream_->codecpar->bit_rate = 128000;
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