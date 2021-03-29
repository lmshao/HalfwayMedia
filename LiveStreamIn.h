//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_LIVESTREAMIN_H
#define HALFWAYLIVE_LIVESTREAMIN_H

#include <thread>
#include "MediaIn.h"
#include "Utils.h"
#include "JitterBuffer.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <boost/asio.hpp>

#include <sys/time.h>
#include <deque>
#include <atomic>

static inline int64_t currentTimeMillis()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class TimeoutHandler {
  public:
    explicit TimeoutHandler(int32_t timeout = 100000) : _valid(true), _timeout(timeout), _lastTime(currentTimeMillis())
    {
    }

    void reset(int32_t timeout)
    {
        _timeout = timeout;
        _lastTime = currentTimeMillis();
    }

    void stop() { _valid = false; }

    static int checkInterrupt(void *handler) { return handler && static_cast<TimeoutHandler *>(handler)->isTimeout(); }

  private:
    bool isTimeout() const
    {
        int32_t delay = currentTimeMillis() - _lastTime;
        return !_valid || delay > _timeout;
    }

    bool _valid;
    int32_t _timeout;
    int64_t _lastTime;
};

class LiveStreamIn : public MediaIn, public JitterBufferListener {
    static const uint32_t DEFAULT_UDP_BUF_SIZE = 8 * 1024 * 1024;

  public:
    explicit LiveStreamIn(const std::string &url);
    ~LiveStreamIn() override;

    bool open() override;

  public:
    void onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt) override;
    void onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp) override;

    void deliverNullVideoFrame();
    void deliverVideoFrame(AVPacket *pkt) override;
    void deliverAudioFrame(AVPacket *pkt) override;
    void start() override;

  private:
    bool isRtsp() { return (_url.compare(0, 7, "rtsp://") == 0); }

    bool isFileInput()
    {
        return (_url.compare(0, 7, "file://") == 0 || _url.compare(0, 1, "/") == 0 || _url.compare(0, 1, ".") == 0);
    }

    void requestKeyFrame()
    {
        if (!_keyFrameRequest)
            _keyFrameRequest = true;
    }

    bool connect();
    bool reconnect();
    void receiveLoop();

    void checkVideoBitstream(AVStream *st, const AVPacket *pkt);
    bool parseAVCC(AVPacket *pkt);
    bool filterVBS(AVStream *st, AVPacket *pkt);
    bool filterPS(AVStream *st, AVPacket *pkt);

    AVDictionary *_options{};

    TimeoutHandler *_timeoutHandler{};

    AVRational _msTimeBase{};
    AVRational _videoTimeBase{};
    AVRational _audioTimeBase{};

    bool _isFileInput;
    int64_t _timstampOffset{};
    int64_t _lastTimstamp{};

  private:
    std::shared_ptr<JitterBuffer> _videoJitterBuffer;
    std::shared_ptr<JitterBuffer> _audioJitterBuffer;
};

#endif  // HALFWAYLIVE_LIVESTREAMIN_H
