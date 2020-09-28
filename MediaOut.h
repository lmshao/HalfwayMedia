//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAOUT_H
#define HALFWAYLIVE_MEDIAOUT_H

#include <iostream>
#include <thread>
#include <queue>

extern "C" {
#include <libavformat/avformat.h>
}
#include "MediaFramePipeline.h"

class MediaFrame {
public:
    explicit MediaFrame(const Frame &frame, int64_t timestamp = 0);
    ~MediaFrame();

    int64_t _timestamp;
    int64_t _duration;
    Frame _frame;
};

class MediaFrameQueue {
public:
    MediaFrameQueue();
    virtual ~MediaFrameQueue() = default;
    void pushFrame(const Frame &frame);
    std::shared_ptr<MediaFrame> popFrame(int timeout = 0);
    void cancel();

private:
    std::queue<std::shared_ptr<MediaFrame>> _queue;
    std::mutex _mutex;
    std::condition_variable _cond;

    std::shared_ptr<MediaFrame> _lastAudioFrame;
    std::shared_ptr<MediaFrame> _lastVideoFrame;

    bool _valid{};
    int64_t _startTimeOffset{};
};

class MediaOut : public FrameDestination {
public:
    explicit MediaOut(const std::string &url, bool hasAudio, bool hasVideo);
    virtual ~MediaOut();
    virtual void onFrame(const Frame &frame);

protected:
    virtual bool isAudioFormatSupported(FrameFormat format) = 0;
    virtual bool isVideoFormatSupported(FrameFormat format) = 0;
    virtual const char *getFormatName(std::string &url) = 0;
    virtual uint32_t getKeyFrameInterval(void) = 0;
    virtual uint32_t getReconnectCount(void) = 0;

    virtual void sendLoop() = 0;

    void close();

private:
    std::string _url;
    bool _hasAudio;
    bool _hasVideo;

    FrameFormat _audioFormat;
    uint32_t _sampleRate;
    uint32_t _channels;
    FrameFormat _videoFormat;
    uint32_t _width;
    uint32_t _height;

    MediaFrameQueue _frameQueue;

    AVFormatContext *_avFmtCtx;
    AVStream *_audioStream;
    AVStream *_videoStream;

    std::thread _thread;
};

#endif  // HALFWAYLIVE_MEDIAOUT_H
