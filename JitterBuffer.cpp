//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "JitterBuffer.h"
#include <thread>

FramePacket::FramePacket(AVPacket *packet) : _packet(nullptr)
{
    _packet = (AVPacket *)malloc(sizeof(AVPacket));
    av_init_packet(_packet);
    av_packet_ref(_packet, packet);
}

FramePacket::~FramePacket()
{
    if (_packet) {
        av_packet_unref(_packet);
        free(_packet);
        _packet = nullptr;
    }
}

void FramePacketBuffer::pushPacket(std::shared_ptr<FramePacket> &framePacket)
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    _queue.push_back(framePacket);
    if (_queue.size() == 1)
        _queueCond.notify_one();
}

std::shared_ptr<FramePacket> FramePacketBuffer::popPacket(bool noWait)
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    std::shared_ptr<FramePacket> packet;
    while (!noWait && _queue.empty()) {
        _queueCond.wait(lock);
    }

    if (!_queue.empty()) {
        packet = _queue.front();
        _queue.pop_front();
    }

    return packet;
}

std::shared_ptr<FramePacket> FramePacketBuffer::frontPacket(bool noWait)
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    std::shared_ptr<FramePacket> packet;

    while (!noWait && _queue.empty()) {
        _queueCond.wait(lock);
    }

    if (!_queue.empty()) {
        packet = _queue.front();
    }

    return packet;
}

std::shared_ptr<FramePacket> FramePacketBuffer::backPacket(bool noWait)
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    std::shared_ptr<FramePacket> packet;

    while (!noWait && _queue.empty()) {
        _queueCond.wait(lock);
    }

    if (!_queue.empty()) {
        packet = _queue.back();
    }

    return packet;
}

uint32_t FramePacketBuffer::size()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    return _queue.size();
}

void FramePacketBuffer::clear()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    _queue.clear();
}

JitterBuffer::JitterBuffer(std::string name, SyncMode syncMode, JitterBufferListener *listener, int64_t maxBufferingMs)
  : _name(name),
    _syncMode(syncMode),
    _isClosing(false),
    _isRunning(false),
    _lastInterval(5),
    _isFirstFramePacket(true),
    _listener(listener),
    _syncTimestamp(AV_NOPTS_VALUE),
    _firstTimestamp(AV_NOPTS_VALUE),
    _maxBufferingMs(maxBufferingMs)
{
}

JitterBuffer::~JitterBuffer()
{
    stop();
}

void JitterBuffer::start(uint32_t delay)
{
    if (!_isRunning) {
        logger("(%s)start", _name.c_str());
        _timer.reset(new boost::asio::deadline_timer(_ioService));
        _timer->expires_from_now(boost::posix_time::milliseconds(delay));
        //_timer->async_wait(boost::bind(&JitterBuffer::onTimeout, this, boost::asio::placeholders::error));
        _timer->async_wait([&](const boost::system::error_code &ec) { onTimeout(ec); });
        _timingThread.reset(new std::thread([ObjectPtr = &_ioService] { ObjectPtr->run(); }));
        _isRunning = true;
    }
}

void JitterBuffer::stop()
{
    if (_isRunning) {
        logger("(%s)stop", _name.c_str());
        _timer->cancel();

        _isClosing = true;
        _timingThread->join();
        _buffer.clear();
        _ioService.reset();
        _isRunning = false;
        _isClosing = false;

        _isFirstFramePacket = true;
        _syncTimestamp = AV_NOPTS_VALUE;
        _syncLocalTime.reset();
        _firstTimestamp = AV_NOPTS_VALUE;
        _firstLocalTime.reset();
    }
}

void JitterBuffer::drain()
{
    logger("(%s)drain jitter buffer size(%d)", _name.c_str(), _buffer.size());

    while (_buffer.size() > 0) {
        logger("(%s)drain jitter buffer, size(%d) ...", _name.c_str(), _buffer.size());
        usleep(10);
    }
}

uint32_t JitterBuffer::sizeInMs()
{
    int64_t bufferingMs = 0;
    std::shared_ptr<FramePacket> frontPacket = _buffer.frontPacket();
    std::shared_ptr<FramePacket> backPacket = _buffer.backPacket();

    if (frontPacket && backPacket) {
        bufferingMs = backPacket->getAVPacket()->dts - frontPacket->getAVPacket()->dts;
    }
    return bufferingMs;
}

void JitterBuffer::onTimeout(const boost::system::error_code &ec)
{
    // Cyclic timing task
    if (!ec) {
        if (!_isClosing)
            handleJob();
    }
}

void JitterBuffer::insert(AVPacket &pkt)
{
    std::shared_ptr<FramePacket> framePacket(new FramePacket(&pkt));
    _buffer.pushPacket(framePacket);
}

void JitterBuffer::setSyncTime(int64_t &syncTimestamp, boost::posix_time::ptime &syncLocalTime)
{
    std::unique_lock<std::mutex> lock(_syncMutex);

    logger("(%s)set sync timestamp %ld -> %ld", _name.c_str(), _syncTimestamp, syncTimestamp);

    _syncTimestamp = syncTimestamp;
    _syncLocalTime.reset(new boost::posix_time::ptime(syncLocalTime));
}

int64_t JitterBuffer::getNextTime(AVPacket *pkt)
{
    int64_t interval = _lastInterval;
    int64_t diff = 0;
    int64_t timestamp = 0;
    int64_t nextTimestamp = 0;

    std::shared_ptr<FramePacket> nextFramePacket = _buffer.frontPacket();
    AVPacket *nextPkt = nextFramePacket != nullptr ? nextFramePacket->getAVPacket() : nullptr;

    if (!pkt || !nextPkt) {
        interval = 10;
        logger("(%s)no next frame, next time %ld", _name.c_str(), interval);
        return interval;
    }

    timestamp = pkt->dts;
    nextTimestamp = nextPkt->dts;

    diff = nextTimestamp - timestamp;
    if (diff < 0 || diff > 2000) {  // revised
        logger("(%s)timestamp rollback, %ld -> %ld", _name.c_str(), timestamp, nextTimestamp);
        if (_syncMode == SYNC_MODE_MASTER)
            _listener->onSyncTimeChanged(this, nextTimestamp);

        logger("(%s)reset first timestamp %ld -> %ld", _name.c_str(), _firstTimestamp, nextTimestamp);
        _firstTimestamp = nextTimestamp;
        _firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time() +
                                                           boost::posix_time::milliseconds(interval)));
        return interval;
    }

    {
        std::unique_lock<std::mutex> lock(_syncMutex);
        if (_isFirstFramePacket) {
            logger("(%s)set first timestamp %ld -> %ld", _name.c_str(), _firstTimestamp, timestamp);
            _firstTimestamp = timestamp;
            _firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time()));

            if (_syncTimestamp == AV_NOPTS_VALUE && _syncMode == SYNC_MODE_MASTER) {
                _syncMutex.unlock();
                _listener->onSyncTimeChanged(this, timestamp);
                _syncMutex.lock();
            }

            _isFirstFramePacket = false;
        }

        boost::posix_time::ptime mst = boost::posix_time::microsec_clock::local_time();
        if (_syncTimestamp != AV_NOPTS_VALUE) {
            // sync timestamp changed
            if (nextTimestamp < _syncTimestamp) {
                logger("(%s)timestamp(%ld) is behind sync timestamp(%ld), diff %ld!", _name.c_str(), nextTimestamp,
                       _syncTimestamp, nextTimestamp - _syncTimestamp);
                interval = diff;
            } else {
                interval = (*_syncLocalTime - mst).total_milliseconds() + (nextTimestamp - _syncTimestamp);
            }
        } else {
            interval = (*_firstLocalTime - mst).total_milliseconds() + (nextTimestamp - _firstTimestamp);
        }

        if (_syncMode == SYNC_MODE_MASTER) {
            if (interval < 0) {
                _syncMutex.unlock();
                _listener->onSyncTimeChanged(this, timestamp);
                _syncMutex.lock();

                logger("(%s)force next time %ld -> %ld", _name.c_str(), interval, _lastInterval);
                interval = _lastInterval;
            } else if (interval > 1000) {
                logger("(%s)force next time %ld -> %ld", _name.c_str(), interval, 1000l);
                interval = 1000;
            }
        } else {
            if (interval < 0) {
                logger("(%s)force next time %ld -> %ld", _name.c_str(), interval, 0l);
                interval = 0;
            } else if (interval > 1000) {
                logger("(%s)force next time %ld -> %ld", _name.c_str(), interval, 1000l);
                interval = 1000;
            }
        }

        _lastInterval = (_lastInterval * 4.0 + interval) / 5;
        return interval;
    }
}

void JitterBuffer::handleJob()
{
    std::shared_ptr<FramePacket> framePacket;

    // if buffering frames exceed maxBufferingMs, do seek to maxBufferingMs / 2
    std::shared_ptr<FramePacket> frontPacket = _buffer.frontPacket();
    std::shared_ptr<FramePacket> backPacket = _buffer.backPacket();
    if (frontPacket && backPacket) {
        int64_t bufferingMs = backPacket->getAVPacket()->dts - frontPacket->getAVPacket()->dts;
        if (bufferingMs > _maxBufferingMs) {
            logger("(%s)Do seek, bufferingMs(%ld), maxBufferingMs(%ld), QueueSize(%d)", _name.c_str(), bufferingMs,
                   _maxBufferingMs, _buffer.size());
            int64_t seekMs = backPacket->getAVPacket()->dts - _maxBufferingMs / 2;
            while (true) {
                framePacket = _buffer.frontPacket();
                if (!framePacket || framePacket->getAVPacket()->dts > seekMs)
                    break;

                _listener->onDeliverFrame(this, framePacket->getAVPacket());
                _buffer.popPacket();
            }

            _syncMutex.lock();
            _firstTimestamp = seekMs;
            _firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time()));
            _syncMutex.unlock();

            if (_syncMode == SYNC_MODE_MASTER) {
                _listener->onSyncTimeChanged(this, seekMs);
            }

            logger("(%s)After seek, QueueSize(%d)", _name.c_str(), _buffer.size());
        }
    }

    // next frame
    uint32_t interval;

    framePacket = _buffer.popPacket();
    AVPacket *pkt = framePacket != nullptr ? framePacket->getAVPacket() : nullptr;

    interval = getNextTime(pkt);
    _timer->expires_from_now(boost::posix_time::milliseconds(interval));

    if (pkt != nullptr)
        _listener->onDeliverFrame(this, pkt);
    else
        logger("(%s)no frame in JitterBuffer", _name.c_str());

    logger("(%s)buffer size %d, next time %d", _name.c_str(), _buffer.size(), interval);
    _timer->async_wait([&](const boost::system::error_code &ec) { onTimeout(ec); });
}