//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "RawFileIn.h"

#include <unistd.h>

#include "AudioTime.h"

RawFileIn::RawFileIn(const std::string &filename, const RawFileInfo &info, bool liveMode)
    : MediaIn(filename), _file(nullptr), _buffLength(0), isLiveMode(liveMode)
{
    if (info.type == "video") {
        _videoHeight = info.media.video.height;
        _videoWidth = info.media.video.width;
        _videoFormat = FRAME_FORMAT_I420;  // just support YUV420
        _videoFramerate = info.media.video.framerate;
        _videoStreamIndex = 0;
    } else if (info.type == "audio") {
        _audioChannels = info.media.audio.channel;
        _audioFormat = FRAME_FORMAT_PCM_48000_2;
        _audioSampleRate = info.media.audio.sample_rate;
        _audioStreamIndex = 0;
    }

    _file = fopen(filename.c_str(), "rb");
}

RawFileIn::~RawFileIn()
{
    if (_file) {
        fclose(_file);
    }
}

bool RawFileIn::open()
{
    if (!_file) {
        _file = fopen(_url.c_str(), "rb");
    }

    _runing = true;
    return true;
}

void RawFileIn::start()
{
    if (_file) {
        _thread = std::thread(&RawFileIn::readFileLoop, this);
    }
}

void RawFileIn::readFileLoop()
{
    unsigned int frameSize;
    std::chrono::microseconds interval;

    if (_videoStreamIndex != -1) {
        frameSize = (_videoWidth * _videoHeight * 3 / 2);  // for YUV420
        interval = std::chrono::microseconds((long)(1000000 / _videoFramerate));
    } else if (_audioStreamIndex != -1) {
        interval = std::chrono::microseconds(1024 * 1000000 / _audioSampleRate);  // 1024 nb_samples
        // 2byte/16bit sampling depth for s16le, 4byte for f32le
        frameSize = 1024 * 2 * 2;  // 1024 * 2ch * 2byte Depth
    } else {
        return;
    }

    logger("frame size = %d, interval = %d ms\n", frameSize, interval.count() / 1000);

    if (_buff == nullptr) {
        _buff.reset(new uint8_t[frameSize]);
        _buffLength = (int)frameSize;
    }

    long n = 0;
    auto start = std::chrono::system_clock::now();
    std::chrono::time_point next = start;

    size_t bytes;
    while (_runing && _file) {
        bytes = fread(_buff.get(), 1, frameSize, _file);
        if (bytes != frameSize) {
            if (bytes != 0) {
                logger("fread error:%s", strerror(errno));
            } else {
                logger("End of File");
            }
            break;
        }

        if (_videoStreamIndex != -1) {
            deliverVideoFrame();
        } else {
            deliverAudioFrame();
        }

        // read file by frame rate
        if (isLiveMode) {
            next = start + interval * ++n;
            std::this_thread::sleep_until(next);
        }
    }
}

void RawFileIn::deliverVideoFrame()
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _videoFormat;
    frame.payload = _buff.get();
    frame.length = _buffLength;
    frame.timeStamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    frame.additionalInfo.video.width = _videoWidth;
    frame.additionalInfo.video.height = _videoHeight;
    frame.additionalInfo.video.isKeyFrame = true;
    deliverFrame(frame);
    logger("deliver video frame, timestamp %ld(%ld), size %4d", frame.timeStamp, frame.timeStamp, frame.length);
}

void RawFileIn::deliverAudioFrame()
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _audioFormat;
    frame.payload = _buff.get();
    frame.length = _buffLength;
    frame.timeStamp =
        AudioTime::currentTime(); /*std::chrono::system_clock::now().time_since_epoch().count() / 1000000;*/
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.sampleRate = _audioSampleRate;
    frame.additionalInfo.audio.channels = _audioChannels;
    frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels / 2;
    deliverFrame(frame);

    logger("deliver audio frame, timestamp %ld(%ld), size %4d", frame.timeStamp, frame.timeStamp, frame.length);
}
