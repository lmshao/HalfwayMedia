//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "VideoEncoder.h"

#include <assert.h>

#include "Utils.h"

VideoEncoder::VideoEncoder(FrameFormat format)
    : _valid(true),
      _width(0),
      _height(0),
      _frameRate(0),
      _bitrateKbps(0),
      _keyFrameIntervalSeconds(0),
      _format(format),
      _videoEnc(nullptr),
      _videoFrame(nullptr),
      _videoPkt(nullptr),
      _ic(nullptr)
{
}

VideoEncoder::~VideoEncoder()
{
    flushEncoder();

    if (!_valid)
        return;

    if (_videoFrame) {
        av_frame_free(&_videoFrame);
    }

    if (_videoPkt) {
        av_packet_free(&_videoPkt);
    }

    if (_videoEnc) {
        avcodec_free_context(&_videoEnc);
    }

    delete _ic;

    _format = FRAME_FORMAT_UNKNOWN;
}

void VideoEncoder::onFrame(const Frame &frame)
{
    std::shared_lock<std::shared_mutex> sharedLock(_mutex);

    if (frame.format != FRAME_FORMAT_I420 && frame.format != FRAME_FORMAT_BGRA) {
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

    if (frame.format == FRAME_FORMAT_I420) {
        av_image_fill_arrays(_videoFrame->data, _videoFrame->linesize, frame.payload, AV_PIX_FMT_YUV420P, _width,
                             _height, 1);
    } else if (frame.format == FRAME_FORMAT_BGRA) {
        if (!_ic) {
            _ic = new ImageConversion;
            _icData.reset(new DataBuffer(_width * _height * 3 / 2));
        }

        _ic->BGRA32ToYUV420(frame.payload, _icData->data, frame.additionalInfo.video.width,
                            frame.additionalInfo.video.height);

        av_image_fill_arrays(_videoFrame->data, _videoFrame->linesize, _icData->data, AV_PIX_FMT_YUV420P, _width,
                             _height, 1);
    }

    _videoFrame->pts = frame.timeStamp;

    avcodec_send_frame(_videoEnc, _videoFrame);
    int ret = avcodec_receive_packet(_videoEnc, _videoPkt);
    if (ret) {
        logger("avcodec_receive_packet, %s", ff_err2str(ret));
        return;
    }

    sendOut(*_videoPkt);
    av_packet_unref(_videoPkt);
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
    _videoEnc->time_base = {1, 24};
    _videoEnc->framerate = {24, 1};
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

    // TODO: remove av_frame_alloc() if use av_image_fill_arrays()
    _videoFrame = av_frame_alloc();
    if (!_videoFrame) {
        logger("Cannot allocate audio frame");
        goto fail;
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
    if (_videoPkt) {
        av_packet_free(&_videoPkt);
    }

    _videoPkt = av_packet_alloc();
    if (!_videoPkt) {
        logger("Cannot allocate audio frame");
        goto fail;
    }

    return true;

fail:
    if (_videoFrame) {
        av_frame_free(&_videoFrame);
    }

    if (_videoPkt) {
        av_packet_free(&_videoPkt);
    }

    avcodec_free_context(&_videoEnc);

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
    while (true) {
        avcodec_send_frame(_videoEnc, nullptr);
        int ret = avcodec_receive_packet(_videoEnc, _videoPkt);
        if (ret == 0) {
            sendOut(*_videoPkt);
        } else if (ret == AVERROR_EOF) {
            logger("recv pkt over");
            break;
        } else {
            logger("flush encoder", ff_err2str(ret));
            break;
        }
    }

    av_packet_unref(_videoPkt);
}
