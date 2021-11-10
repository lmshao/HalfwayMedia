//
// Created by lmshao on 2020/10/18.
// Copyright (c) 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "VideoTranscoder.h"

#include <cassert>

VideoTranscoder::VideoTranscoder() {}
VideoTranscoder::~VideoTranscoder() {}

void VideoTranscoder::onFrame(const Frame &frame)
{
    deliverFrame(frame);
}

bool VideoTranscoder::setInput(int input, FrameFormat format, FrameSource *source)
{
    assert(source);

    std::shared_lock<std::shared_mutex> sharedLock(_inputMutex);

    if (_inputs.find(input) != _inputs.end())
        return false;

    std::shared_ptr<VideoDecoder> decoder;

    if (!decoder)
        decoder.reset(new VideoDecoder());

    if (!decoder)
        return false;

    if (decoder->init(format)) {
        decoder->addVideoDestination(this);
        source->addVideoDestination(decoder.get());
        sharedLock.unlock();
        std::unique_lock<std::shared_mutex> uniqueLock(_inputMutex);
        Input in{ .source = source, .decoder = decoder };
        _inputs[input] = in;
        return true;
    }

    return false;
}

void VideoTranscoder::unsetInput(int input)
{
    std::shared_lock<std::shared_mutex> sharedLock(_inputMutex);
    if (_inputs.find(input) != _inputs.end()) {
        Input in = _inputs.at(input);
        in.source->removeVideoDestination(in.decoder.get());
        in.decoder->removeVideoDestination(this);
        sharedLock.unlock();
        std::unique_lock<std::shared_mutex> uniqueLock(_inputMutex);
        _inputs.erase(input);
    }
}

bool VideoTranscoder::addOutput(int output, FrameFormat frameFormat, unsigned int width, unsigned int height,
                                const unsigned int framerateFPS, const unsigned int bitrateKbps, FrameDestination *dest)
{
    std::shared_ptr<VideoEncoder> encoder;
    //    std::shared_ptr<VideoProcesser> processer;

    std::shared_lock<std::shared_mutex> sharedLock(_outputMutex);

    int32_t streamId = -1;

    if (!encoder)
        encoder.reset(new VideoEncoder(frameFormat));

    if (!encoder)
        return false;

    streamId = encoder->generateStream(width, height, dest);
    if (streamId < 0)
        return false;

    //    if (!processer) {
    //        processer.reset(new owt_base::FrameProcesser());
    //    }
    //    if (!processer->init(encoder->getInputFormat(), rootSize.width, rootSize.height, framerateFPS))
    //        return false;

    this->addVideoDestination(encoder.get());
    sharedLock.unlock();
    std::unique_lock<std::shared_mutex> uniqueLock(_outputMutex);
    Output out{ .streamId = streamId, .encoder = encoder };
    _outputs[output] = out;
    return false;
}

void VideoTranscoder::removeOutput(int output)
{
    std::shared_lock<std::shared_mutex> sharedLock(_outputMutex);
    if (_outputs.find(output) != _outputs.end()) {
        Output out = _outputs.at(output);

        out.encoder->degenerateStream(out.streamId);
        this->removeVideoDestination(out.encoder.get());

        sharedLock.unlock();
        std::unique_lock<std::shared_mutex> uniqueLock(_inputMutex);
        _outputs.erase(output);
    }
}
