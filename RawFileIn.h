//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYMEDIA_RAWFILEIN_H
#define HALFWAYMEDIA_RAWFILEIN_H

#include "MediaIn.h"
#include "Utils.h"

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

struct VideoInfo {
    char *pix_fmt;  // support yuv420 AV_PIX_FMT_YUV420P | bgra AV_PIX_FMT_BGRA
    int width;
    int height;
    int framerate;
};

struct AudioInfo {
    char *sample_fmt;  // PCM s16le: AV_SAMPLE_FMT_S16, f32le: AV_SAMPLE_FMT_FLT
    int channel;
    int sample_rate;
};

struct RawFileInfo {
    std::string type;
    union {
        VideoInfo video;
        AudioInfo audio;
    } media;
};

class RawFileIn : public MediaIn {
  public:
    RawFileIn(const std::string &filename, const RawFileInfo &info, bool liveMode = false);
    virtual ~RawFileIn();

    bool open() override;

    void deliverVideoFrame(AVPacket *pkt) override{};
    void deliverVideoFrame();

    void deliverAudioFrame(AVPacket *pkt) override{};
    void deliverAudioFrame();

    void start() override;

  private:
    void readFileLoop();

    FILE *_file;
    int _videoFramerate;
    //    std::shared_ptr<uint8_t[]> _buff;
    //    int _buffLength;
    DataBuffer *_buff;
    bool isLiveMode;
};

#endif  // HALFWAYMEDIA_RAWFILEIN_H
