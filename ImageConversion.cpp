//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "ImageConversion.h"

#include "Utils.h"

ImageConversion::ImageConversion() : _srcFrame(nullptr), _dstFrame(nullptr), _swsCtx(nullptr), _inited(false) {}

ImageConversion::~ImageConversion()
{
    if (!_srcFrame)
        av_frame_free(&_srcFrame);

    if (!_dstFrame)
        av_frame_free(&_dstFrame);

    if (!_swsCtx) {
        sws_freeContext(_swsCtx);
        _swsCtx = nullptr;
    }
}

bool ImageConversion::BGRA32ToYUV420(uint8_t *bgra, uint8_t *yuv, int width, int height)
{
    if (!_inited) {
        init(AV_PIX_FMT_BGRA, width, height, AV_PIX_FMT_YUV420P, width, height);
        if (!_inited)
            return false;
    }

    av_image_fill_arrays(_srcFrame->data, _srcFrame->linesize, bgra, AV_PIX_FMT_BGRA, width, height, 1);

    // Check if context can be reused, otherwise reallocate a new one.
    _swsCtx = sws_getCachedContext(_swsCtx, width, height, AV_PIX_FMT_BGRA, width, height, AV_PIX_FMT_YUV420P,
                                   SWS_BILINEAR, nullptr, nullptr, nullptr);

    int res = sws_scale(_swsCtx, _srcFrame->data, _srcFrame->linesize, 0, height, _dstFrame->data, _dstFrame->linesize);
    if (res != height) {
        logger("sws_scale error");
        return false;
    }

    // The size of yuv buff must be >=  (width * height * 3/2)
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);  // width * height * 3/2
    av_image_copy_to_buffer(yuv, size, _dstFrame->data, _dstFrame->linesize, AV_PIX_FMT_YUV420P, width, height, 1);

    return true;
}

bool ImageConversion::init(AVPixelFormat srcFormat, int srcW, int srcH, AVPixelFormat dstFormat, int dstW, int dstH)
{
    if (!_srcFrame) {
        _srcFrame = av_frame_alloc();
    }

    if (!_dstFrame) {
        _dstFrame = av_frame_alloc();
    }

    if (!_swsCtx) {
        _swsCtx = sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    if (!_srcFrame || !_dstFrame || !_swsCtx) {
        logger("init error");
        return false;
    }

    // alloc image & bind it to th dst av frame
    int res = av_image_alloc(_dstFrame->data, _dstFrame->linesize, dstW, dstH, AV_PIX_FMT_YUV420P, 1);
    if (res < 0) {
        logger("%s", ff_err2str(res));
        return false;
    }

    _inited = true;
    return true;
}

// just for test bgra to yuv
void ImageConversion::onFrame(const Frame &frame)
{
    logger("on frame size = %d", frame.length);
    DUMP_HEX(frame.payload, 10);

    Frame frame1 = frame;
    frame1.format = FRAME_FORMAT_I420;
    int outSize = frame.additionalInfo.video.width * frame.additionalInfo.video.height * 3 / 2;
    std::shared_ptr<uint8_t[]> buff(new uint8_t[outSize]);

    bool ok =
        BGRA32ToYUV420(frame.payload, buff.get(), frame.additionalInfo.video.width, frame.additionalInfo.video.height);
    if (!ok)
        return;

    frame1.payload = buff.get();
    frame1.length = outSize;
    deliverFrame(frame1);
}
