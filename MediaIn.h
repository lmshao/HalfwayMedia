//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAIN_H
#define HALFWAYLIVE_MEDIAIN_H

#include <string>
#include <thread>
#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
}

class MediaIn : public FrameSource {
public:
    explicit MediaIn(const std::string &filename);
    virtual ~MediaIn();
    virtual bool open() = 0;
    bool hasAudio() const;
    bool hasVideo() const;

    virtual void deliverVideoFrame(AVPacket *pkt) = 0;
    virtual void deliverAudioFrame(AVPacket *pkt) = 0;

    virtual void start() = 0;

protected:
    std::string _filename;
    AVFormatContext *_avFmtCtx;

    int _audioStreamIndex;
    FrameFormat _audioFormat;
    uint32_t _audioSampleRate;
    uint32_t _audioChannels;

    int _videoStreamIndex;
    FrameFormat _videoFormat;
    uint32_t _videoWidth;
    uint32_t _videoHeight;

private:
    std::thread _thread;
};

#endif  // HALFWAYLIVE_MEDIAIN_H
