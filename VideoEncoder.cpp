//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "VideoEncoder.h"
#include "Utils.h"
#include <assert.h>

VideoEncoder::VideoEncoder(FrameFormat format)
  : _valid(true), _width(0), _height(0), _frameRate(0), _bitrateKbps(0), _keyFrameIntervalSeconds(0), _format(format)
{
}

VideoEncoder::~VideoEncoder()
{
    flushEncoder();

    if (!_valid)
        return;

    if (_videoFrame) {
        av_frame_free(&_videoFrame);
        _videoFrame = nullptr;
    }

    if (_videoEnc) {
        avcodec_close(_videoEnc);
        _videoEnc = nullptr;
    }

    _format = FRAME_FORMAT_UNKNOWN;
}

void VideoEncoder::onFrame(const Frame &frame)
{
    std::shared_lock<std::shared_mutex> sharedLock(_mutex);

    if (frame.format != FRAME_FORMAT_I420) {
        logger("Unsupported format %s", getFormatStr(frame.format));
        return;
    }

    if (_width == 0 || _height == 0) {
        _width = frame.additionalInfo.video.width;
        _height = frame.additionalInfo.video.height;
        if (_bitrateKbps == 0) {
            if (_width >= 1920)
                _bitrateKbps = 2048;
            else
                _bitrateKbps = 1024;
        }

        // TODO: init encoder
        if (!initEncoder(_format)) {
            logger("Faild to init video Encoder");
            return;
        }

        _valid = true;
    }

    if (!_valid) {
        logger("need to init encoder");
        return;
    }

    if (av_frame_make_writable(_videoFrame)) {
        return;
    }

    int ySize = _width * _height;
    int uSize = ((_width + 1) / 2) * ((_height + 1) / 2);
    assert(frame.length == ySize + uSize * 2);
    memcpy(_videoFrame->data[0], frame.payload, ySize);
    memcpy(_videoFrame->data[1], frame.payload + ySize, uSize);
    memcpy(_videoFrame->data[2], frame.payload + ySize + uSize, uSize);
    _videoFrame->pts = frame.timeStamp;

    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);

    avcodec_send_frame(_videoEnc, _videoFrame);
    int ret = avcodec_receive_packet(_videoEnc, &pkt);
    if (ret) {
        logger("avcodec_receive_packet, %s", ff_err2str(ret));
        return;
    }

    sendOut(pkt);
    av_packet_unref(&pkt);
}

int32_t VideoEncoder::generateStream(uint32_t width, uint32_t height, FrameDestination *dest)
{
    std::unique_lock<std::shared_mutex> uniqueLock(_mutex);
    logger("generateStream: {.width=%d, .height=%d, .frameRate=d, .bitrateKbps=d, .keyFrameIntervalSeconds=d}", width,
           height);

    return 0;
}

void VideoEncoder::degenerateStream(int32_t streamId) {}

bool VideoEncoder::initEncoder(FrameFormat format)
{
    assert(_width != 0);
    assert(_height != 0);
    assert(_format != FRAME_FORMAT_UNKNOWN);

    int ret;
    AVCodec *codec;

    switch (format) {
        case FRAME_FORMAT_H264:
            codec = avcodec_find_encoder_by_name("libx264");
            if (!codec) {
                logger("Can not find video encoder ffmpeg/libx264, use h264 instead");
                codec = avcodec_find_encoder(AV_CODEC_ID_H264);
                if (!codec) {
                    logger("Can not find video encoder ffmpeg/h264");
                    return false;
                }
            }
            break;

        case FRAME_FORMAT_H265:
            codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
            if (!codec) {
                logger("Could not find video encoder %s", avcodec_get_name(AV_CODEC_ID_HEVC));
                return false;
            }
            break;

        default:
            logger("Encoder %s is not supported", getFormatStr(format));
            return false;
    }

    _videoEnc = avcodec_alloc_context3(codec);
    if (!_videoEnc) {
        logger("Can not alloc avcodec context");
        return false;
    }

    _videoEnc->width = _width;
    _videoEnc->height = _height;
    _videoEnc->bit_rate = _bitrateKbps * 1024;
    _videoEnc->gop_size = 10;
    _videoEnc->time_base = { 1, 24 };
    _videoEnc->framerate = { 24, 1 };
    _videoEnc->max_b_frames = 0;
    _videoEnc->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(_videoEnc->priv_data, "tune", "zerolatency", 0);  // no delay
        //        av_opt_set(_videoEnc->priv_data, "preset", "slow", 0);   // delay ~18 frames
    }

    ret = avcodec_open2(_videoEnc, codec, nullptr);
    if (ret < 0) {
        logger("Cannot open output video codec, %s", ff_err2str(ret));
        return false;
    }

    if (_videoFrame) {
        av_frame_free(&_videoFrame);
    }

    _videoFrame = av_frame_alloc();
    if (!_videoFrame) {
        logger("Cannot allocate audio frame");
        return false;
    }

    _videoFrame->width = _width;
    _videoFrame->height = _height;
    _videoFrame->format = AV_PIX_FMT_YUV420P;

    ret = av_frame_get_buffer(_videoFrame, 0);
    if (ret < 0) {
        logger("Cannot get audio frame buffer, %s", ff_err2str(ret));
        goto fail;
    }

    logger("Video encoder width %d, height %d", _videoFrame->width, _videoFrame->height);

    return true;

fail:
    if (_videoFrame) {
        av_frame_free(&_videoFrame);
        _videoFrame = nullptr;
    }

    if (_videoEnc) {
        avcodec_close(_videoEnc);
        _videoEnc = nullptr;
    }

    return false;
}
void VideoEncoder::sendOut(AVPacket &pkt)
{
    logger("");
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _format;
    frame.payload = pkt.data;
    frame.length = pkt.size;
    frame.additionalInfo.video.width = _videoEnc->width;
    frame.additionalInfo.video.height = _videoEnc->height;
    frame.additionalInfo.video.isKeyFrame = pkt.flags & AV_PKT_FLAG_KEY;
    frame.timeStamp = pkt.dts;

    logger("sendOut video frame, timestamp %ld, size %4d, %dx%d, %s", frame.timeStamp, frame.length, _videoEnc->width,
           _videoEnc->height, (pkt.flags & AV_PKT_FLAG_KEY) ? "key" : "non-key");

    deliverFrame(frame);
}
void VideoEncoder::flushEncoder()
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);

    while (true) {
        avcodec_send_frame(_videoEnc, nullptr);
        int ret = avcodec_receive_packet(_videoEnc, &pkt);
        if (ret == 0) {
            sendOut(pkt);
            av_packet_unref(&pkt);
        } else if (ret == AVERROR_EOF) {
            logger("recv pkt over");
            break;
        } else {
            logger("flush encoder", ff_err2str(ret));
            break;
        }
    }
}
