//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_JITTERBUFFER_H
#define HALFWAYLIVE_JITTERBUFFER_H

#include "Utils.h"
#include <iostream>
#include <boost/asio.hpp>

#include <sys/time.h>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
}

// RAII Wrapper of AVPacket
class FramePacket {
  public:
    explicit FramePacket(AVPacket *packet);
    virtual ~FramePacket();

    AVPacket *getAVPacket() { return _packet; }

  private:
    AVPacket *_packet;
};

// Wrapper of double-ended queue for storing FramePacket
class FramePacketBuffer {
  public:
    FramePacketBuffer() = default;
    virtual ~FramePacketBuffer() = default;

    void pushPacket(std::shared_ptr<FramePacket> &FramePacket);
    std::shared_ptr<FramePacket> popPacket(bool noWait = true);

    std::shared_ptr<FramePacket> frontPacket(bool noWait = true);
    std::shared_ptr<FramePacket> backPacket(bool noWait = true);

    uint32_t size();
    void clear();

  private:
    std::mutex _queueMutex;
    std::condition_variable _queueCond;
    std::deque<std::shared_ptr<FramePacket>> _queue;
};

class JitterBuffer;

class JitterBufferListener {
  public:
    virtual void onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt) = 0;
    virtual void onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp) = 0;
};

class JitterBuffer {
  public:
    enum SyncMode {
        SYNC_MODE_MASTER,
        SYNC_MODE_SLAVE,
    };

    JitterBuffer(std::string name, SyncMode syncMode, JitterBufferListener *listener, int64_t maxBufferingMs = 1000);
    virtual ~JitterBuffer();

    void start(uint32_t delay = 0);
    void stop();
    void drain();
    uint32_t sizeInMs();

    void insert(AVPacket &pkt);
    void setSyncTime(int64_t &syncTimestamp, boost::posix_time::ptime &syncLocalTime);

  protected:
    void onTimeout(const boost::system::error_code &ec);
    int64_t getNextTime(AVPacket *pkt);
    void handleJob();

  private:
    std::string _name;
    SyncMode _syncMode;

    std::atomic<bool> _isClosing;
    bool _isRunning;
    int64_t _lastInterval;
    std::atomic<bool> _isFirstFramePacket;
    JitterBufferListener *_listener;

    FramePacketBuffer _buffer;

    std::unique_ptr<std::thread> _timingThread;
    boost::asio::io_service _ioService;
    std::unique_ptr<boost::asio::deadline_timer> _timer;

    std::unique_ptr<boost::posix_time::ptime> _syncLocalTime;
    int64_t _syncTimestamp;
    std::unique_ptr<boost::posix_time::ptime> _firstLocalTime;
    int64_t _firstTimestamp;
    std::mutex _syncMutex;

    int64_t _maxBufferingMs;
};

#endif  // HALFWAYLIVE_JITTERBUFFER_H
