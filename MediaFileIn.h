//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAFILEIN_H
#define HALFWAYLIVE_MEDIAFILEIN_H

#include <thread>
#include "MediaIn.h"
#include "Utils.h"

extern "C" {
#include <libavformat/avformat.h>
}

class MediaFileIn : public MediaIn {
public:
    explicit MediaFileIn(const std::string &filename);
    ~MediaFileIn() override;

    bool open() override;
    void deliverVideoFrame(AVPacket *pkt) override;
    void deliverAudioFrame(AVPacket *pkt) override;

private:
    bool checkStream();
    void receiveLoop();

public:
    void start() override;

private:
    AVRational _msTimeBase;
    AVRational _videoTimeBase;
    AVRational _audioTimeBase;
    bool _running;
};

#endif  // HALFWAYLIVE_MEDIAFILEIN_H
