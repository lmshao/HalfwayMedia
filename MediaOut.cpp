//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaOut.h"

#include <assert.h>
#include <sys/time.h>
#include "Utils.h"
#include "rtp/RtpHeader.h"

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

MediaFrame::MediaFrame(const Frame &frame, int64_t timestamp) : _timestamp(timestamp), _duration(0)
{
    _frame = frame;
    if (frame.length > 0) {
        uint8_t *payload = frame.payload;
        uint32_t length = frame.length;

        // unpack audio rtp pack
        if (isAudioFrame(frame) && frame.additionalInfo.audio.isRtpPacket) {
            RTPHeader *rtp = reinterpret_cast<RTPHeader *>(payload);
            uint32_t headerLength = rtp->getHeaderLength();
            assert(length >= headerLength);
            payload += headerLength;
            length -= headerLength;
            _frame.additionalInfo.audio.isRtpPacket = false;
            _frame.length = length;
        }

        _frame.payload = (uint8_t *)malloc(length);
        memcpy(_frame.payload, payload, length);
    } else {
        _frame.payload = nullptr;
    }
}

MediaFrame::~MediaFrame()
{
    if (_frame.payload) {
        free(_frame.payload);
        _frame.payload = nullptr;
    }
}

MediaFrameQueue::MediaFrameQueue() : _valid(true), _startTimeOffset(currentTimeMs()) {}

void MediaFrameQueue::pushFrame(const Frame &frame)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_valid)
        return;

    std::shared_ptr<MediaFrame> lastFrame;
    std::shared_ptr<MediaFrame> mediaFrame(
      new MediaFrame(frame, currentTimeMs() - _startTimeOffset));
    if (isAudioFrame(frame)) {
        if (!_lastAudioFrame) {
            _lastAudioFrame = mediaFrame;
            return;
        }

        // reset frame timestamp
        _lastAudioFrame->_duration = mediaFrame->_timestamp - _lastAudioFrame->_timestamp;
        if (_lastAudioFrame->_duration <= 0) {
            _lastAudioFrame->_duration = 1;
            mediaFrame->_timestamp = _lastAudioFrame->_timestamp + 1;
        }

        lastFrame = _lastAudioFrame;
        _lastAudioFrame = mediaFrame;
    } else {
        if (!_lastVideoFrame) {
            _lastVideoFrame = mediaFrame;
            return;
        }

        _lastVideoFrame->_duration = mediaFrame->_timestamp - _lastVideoFrame->_timestamp;
        if (_lastVideoFrame->_duration <= 0) {
            _lastVideoFrame->_duration = 1;
            mediaFrame->_timestamp = _lastVideoFrame->_timestamp + 1;
        }

        lastFrame = _lastVideoFrame;
        _lastVideoFrame = mediaFrame;
    }

    _queue.push(lastFrame);
    if (_queue.size() == 1) {
        lock.unlock();
        _cond.notify_one();
    }
}

std::shared_ptr<MediaFrame> MediaFrameQueue::popFrame(int timeout)
{
    std::unique_lock<std::mutex> lock(_mutex);
    std::shared_ptr<MediaFrame> mediaFrame;

    if (_valid)
        return nullptr;

    if (_queue.empty() && timeout > 0) {
        _cond.wait_until(lock,
                         std::chrono::system_clock::now() + std::chrono::milliseconds(timeout));
    }

    if (!_queue.empty()) {
        mediaFrame = _queue.front();
        _queue.pop();
    }

    return mediaFrame;
}

void MediaFrameQueue::cancel()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _valid = false;
    _cond.notify_all();
}

MediaOut::MediaOut(const std::string &url, bool hasAudio, bool hasVideo)
  : _url(url), _hasAudio(hasAudio), _hasVideo(hasVideo), _avFmtCtx(nullptr)
{
    logger("url %s, audio %d, video %d", _url.c_str(), _hasAudio, _hasVideo);
    if (!_hasAudio && !_hasVideo) {
        logger("Audio/Video not enabled");
        return;
    }
    _thread = std::thread(&MediaOut::sendLoop, this);
}

MediaOut::~MediaOut()
{
    logger("~");
};

void MediaOut::onFrame(const Frame &frame)
{
    if (isAudioFrame(frame)) {
        if (!_hasAudio) {
            logger("Audio is not enabled");
            return;
        }

        if (!isAudioFormatSupported(frame.format)) {
            logger("Unsupported audio frame format: %s(%d)", getFormatStr(frame.format),
                   frame.format);
            return;
        }

        // fill audio info
        if (_audioFormat == FRAME_FORMAT_UNKNOWN) {
            _sampleRate = frame.additionalInfo.audio.sampleRate;
            _channels = frame.additionalInfo.audio.channels;
            _audioFormat = frame.format;
        }

        if (_audioFormat != frame.format) {
            logger("Expected codec(%s), got(%s)", getFormatStr(_audioFormat),
                   getFormatStr(frame.format));
            return;
        }

        // TODO:check status

        if (_channels != frame.additionalInfo.audio.channels ||
            _sampleRate != frame.additionalInfo.audio.sampleRate) {
            logger("Invalid audio frame channels %d, or sample rate: %d",
                   frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            return;
        }

        _frameQueue.pushFrame(frame);
    } else if (isVideoFrame(frame)) {
        if (!_hasVideo) {
            logger("Video is not enabled");
            return;
        }

        if (!isVideoFormatSupported(frame.format)) {
            logger("Unsupported video frame format: %s(%d)", getFormatStr(frame.format),
                   frame.format);
            return;
        }

        if (_videoFormat == FRAME_FORMAT_UNKNOWN) {
            _width = frame.additionalInfo.video.width;
            _height = frame.additionalInfo.video.height;
            _videoFormat = frame.format;
        }

        if (_videoFormat != frame.format) {
            logger("Expected codec(%s), got(%s)", getFormatStr(_videoFormat),
                   getFormatStr(frame.format));
            return;
        }

        // TODO: support updating video status
        if (_width != frame.additionalInfo.video.width ||
            _height != frame.additionalInfo.video.height) {
            logger("Video resolution changed %dx%d -> %dx%d", _width, _height,
                   frame.additionalInfo.video.width, frame.additionalInfo.video.height);
            return;
        }

        _frameQueue.pushFrame(frame);
    } else {
        logger("Unsupported frame format: %s(%d)", getFormatStr(frame.format), frame.format);
    }
}

void MediaOut::close()
{
    logger("Close %s", _url.c_str());
    _frameQueue.cancel();
    _thread.join();
}
