//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFramePipeline.h"
#include "Utils.h"

FrameSource::~FrameSource()
{
    std::unique_lock<std::shared_mutex> alock(_audioDstsMutex);
    for (auto &_audioDst : _audioDsts) {
        _audioDst->unsetAudioSource();
    }
    alock.unlock();
    _audioDsts.clear();

    std::unique_lock<std::shared_mutex> vlock(_videoDstsMutex);
    for (auto &_videoDst : _videoDsts) {
        _videoDst->unsetVideoSource();
    }
    vlock.unlock();
    _videoDsts.clear();
};

void FrameSource::addAudioDestination(FrameDestination *dst)
{
    std::unique_lock<std::shared_mutex> lock(_audioDstsMutex);
    _audioDsts.push_back(dst);
    lock.unlock();
    dst->setAudioSource(this);
}

void FrameSource::removeAudioDestination(FrameDestination *dst)
{
    std::unique_lock<std::shared_mutex> lock(_audioDstsMutex);
    _audioDsts.remove(dst);
    lock.unlock();
    dst->unsetAudioSource();
}

void FrameSource::addVideoDestination(FrameDestination *dst)
{
    std::unique_lock<std::shared_mutex> lock(_videoDstsMutex);
    _videoDsts.push_back(dst);
    lock.unlock();
    dst->setVideoSource(this);
}

void FrameSource::removeVideoDestination(FrameDestination *dst)
{
    std::unique_lock<std::shared_mutex> lock(_videoDstsMutex);
    _videoDsts.remove(dst);
    lock.unlock();
    dst->unsetVideoSource();
}

void FrameSource::deliverFrame(const Frame &frame)
{
    if (isAudioFrame(frame)) {
        logger("deliverFrame audio");
        std::shared_lock<std::shared_mutex> lock(_audioDstsMutex);
        for (auto &_audioDst : _audioDsts) {
            _audioDst->onFrame(frame);
        }
    } else if (isVideoFrame(frame)) {
        logger("deliverFrame video");
        std::shared_lock<std::shared_mutex> lock(_videoDstsMutex);
        logger("_videoDsts no. = %d", _videoDsts.size());
        for (auto &_videoDst : _videoDsts) {
            logger("~~");
            _videoDst->onFrame(frame);
        }
    } else {
        logger("Unknown frame Type");
    }
}

//=====================================================================

void FrameDestination::setAudioSource(FrameSource *src)
{
    std::unique_lock<std::shared_mutex> lock(_audioSrcMutex);
    _audioSrc = src;
}

void FrameDestination::unsetAudioSource()
{
    std::unique_lock<std::shared_mutex> lock(_audioSrcMutex);
    _audioSrc = nullptr;
}

void FrameDestination::setVideoSource(FrameSource *src)
{
    std::unique_lock<std::shared_mutex> lock(_videoSrcMutex);
    _videoSrc = src;
}

void FrameDestination::unsetVideoSource()
{
    std::unique_lock<std::shared_mutex> lock(_videoSrcMutex);
    _videoSrc = nullptr;
}
