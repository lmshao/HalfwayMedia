//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "VideoDecoder.h"
VideoDecoder::VideoDecoder() : _decCtx(nullptr), _decFrame(nullptr), _i420BufferLength(0) {}

VideoDecoder::~VideoDecoder()
{
    if (_decFrame) {
        av_frame_free(&_decFrame);
    }

    if (_decCtx) {
        avcodec_close(_decCtx);
        _decCtx = nullptr;
    }
}

bool VideoDecoder::init(FrameFormat format)
{
    int ret;
    AVCodecID codecId;
    AVCodec *dec;

    switch (format) {
        case FRAME_FORMAT_H264:
            codecId = AV_CODEC_ID_H264;
            break;

        case FRAME_FORMAT_H265:
            codecId = AV_CODEC_ID_HEVC;
            break;

        case FRAME_FORMAT_VP8:
            codecId = AV_CODEC_ID_VP8;
            break;

        case FRAME_FORMAT_VP9:
            codecId = AV_CODEC_ID_VP9;
            break;

        default:
            logger("Unspported video frame format %s(%d)", getFormatStr(format), format);
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

    memset(&_packet, 0, sizeof(_packet));
    return true;
}

void VideoDecoder::onFrame(const Frame &frame)
{
    int ret;

    av_init_packet(&_packet);
    _packet.data = frame.payload;
    _packet.size = frame.length;

    ret = avcodec_send_packet(_decCtx, &_packet);
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

    logger("decode ok");
    int ySize = _decCtx->width * _decCtx->height;
    int uvSize = ((_decCtx->width + 1) / 2) * ((_decCtx->height + 1) / 2);
    int totalSize = ySize + 2 * uvSize;

    if (_i420Buffer == nullptr || _i420BufferLength != totalSize) {
        _i420BufferLength = totalSize;
        _i420Buffer.reset(new uint8_t[_i420BufferLength]);
    }

    memset(_i420Buffer.get(), 0, _i420BufferLength);
    memcpy(_i420Buffer.get(), _decFrame->data[0], ySize);
    memcpy(_i420Buffer.get() + ySize, _decFrame->data[1], uvSize);
    memcpy(_i420Buffer.get() + ySize + uvSize, _decFrame->data[2], uvSize);

    Frame frameDecoded;
    memset(&frameDecoded, 0, sizeof(frameDecoded));
    frameDecoded.format = FRAME_FORMAT_I420;
    frameDecoded.payload = _i420Buffer.get();
    frameDecoded.length = _i420BufferLength;
    frameDecoded.timeStamp = _decFrame->pkt_dts;
    frameDecoded.additionalInfo.video.width = _decCtx->width;
    frameDecoded.additionalInfo.video.height = _decCtx->height;

    logger("deliverFrame, %dx%d, timeStamp %d", frameDecoded.additionalInfo.video.width,
           frameDecoded.additionalInfo.video.height, frame.timeStamp);
    deliverFrame(frameDecoded);
}
