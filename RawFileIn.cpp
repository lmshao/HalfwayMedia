//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "RawFileIn.h"

#include <string.h>
#include <unistd.h>

#include "AudioTime.h"

RawFileIn::RawFileIn(const std::string &filename, const RawFileInfo &info, bool liveMode)
    : MediaIn(filename), _file(nullptr), _buff(nullptr), isLiveMode(liveMode)
{
    if (info.type == "video") {
        _videoHeight = info.media.video.height;
        _videoWidth = info.media.video.width;

        if (!strcmp(info.media.video.pix_fmt, "bgra")) {
            _videoFormat = FRAME_FORMAT_BGRA;  // support bgra32
        } else {
            _videoFormat = FRAME_FORMAT_I420;  // support YUV420
        }

        _videoFramerate = info.media.video.framerate;
        _videoStreamIndex = 0;
    } else if (info.type == "audio") {
        _audioChannels = info.media.audio.channel;
        _audioFormat = FRAME_FORMAT_PCM_48000_2;
        if (!strcmp(info.media.audio.sample_fmt, "f32le")) {
            _audioFormat = FRAME_FORMAT_PCM_48000_2_FLT;  // support f32le
        } else {
            _audioFormat = FRAME_FORMAT_PCM_48000_2_S16;  // default s16le
        }
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

    if (_buff) {
        delete _buff;
    }
}

bool RawFileIn::open()
{
    if (!_file) {
        _file = fopen(_url.c_str(), "rb");
    }

    if (!_file) {
        logger("error open %s", _url.c_str());
        return false;
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
        if (_videoFormat == FRAME_FORMAT_BGRA) {
            frameSize = _videoWidth * _videoHeight * 4;  // for BGRA32
        } else {
            frameSize = _videoWidth * _videoHeight * 3 / 2;  // for YUV420
        }

        interval = std::chrono::microseconds((long)(1000000 / _videoFramerate));
    } else if (_audioStreamIndex != -1) {
        interval = std::chrono::microseconds(1024 * 1000000 / _audioSampleRate);  // 1024 nb_samples
        // 2byte/16bit sampling depth for s16le, 4byte for f32le
        if (_audioFormat == FRAME_FORMAT_PCM_48000_2_FLT) {
            frameSize = 1024 * 2 * 4;  // 1024 * 2ch * 4byte depth
        } else {
            frameSize = 1024 * 2 * 2;  // 1024 * 2ch * 2byte depth
        }
    } else {
        return;
    }

    logger("frame size = %d, interval = %d ms\n", frameSize, interval.count() / 1000);

    if (_buff == nullptr) {
        _buff = new DataBuffer(frameSize);
    }

    long n = 0;
    auto start = std::chrono::system_clock::now();
    std::chrono::time_point next = start;

    size_t bytes;
    while (_runing && _file) {
        bytes = fread(_buff->data, 1, frameSize, _file);
        if (bytes != frameSize) {
            if (bytes != 0) {
                logger("read frame error [incomplete file] | %s", strerror(errno));
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
    frame.payload = _buff->data;
    frame.length = _buff->length;
    frame.timeStamp = currentTime() - startTime();
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
    frame.payload = _buff->data;
    frame.length = _buff->length;
    frame.timeStamp = currentTime() - startTime();
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.sampleRate = _audioSampleRate;
    frame.additionalInfo.audio.channels = _audioChannels;
    if (_audioFormat == FRAME_FORMAT_PCM_48000_2_FLT) {
        // 4 bytes per sample
        frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels / 4;
    } else {
        frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels / 2;
    }

    deliverFrame(frame);
    logger("deliver audio frame, timestamp %ld(%ld), size %4d", frame.timeStamp, frame.timeStamp, frame.length);
}
