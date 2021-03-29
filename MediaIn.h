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

    void waitThread();

    void close();

  protected:
    std::string _url;
    std::string _enableAudio;
    std::string _enableVideo;

    AVDictionary *_options;
    bool _runing;
    bool _keyFrameRequest;
    std::thread _thread;

    AVFormatContext *_avFmtCtx;
    AVPacket _avPacket;

    int _audioStreamIndex;
    FrameFormat _audioFormat;
    uint32_t _audioSampleRate;
    uint32_t _audioChannels;

    int _videoStreamIndex;
    FrameFormat _videoFormat;
    uint32_t _videoWidth;
    uint32_t _videoHeight;
    bool _needCheckVBS;
    bool _needApplyVBSF;
    AVBSFContext *_vbsf;

    AVRational _msTimeBase;
    AVRational _videoTimeBase;
    AVRational _audioTimeBase;

    bool _isFileInput;
    int64_t _timestampOffset;
    int64_t _lastTimestamp;

    bool _enableVideoExtradata;
    std::shared_ptr<uint8_t[]> _spsPpsBuffer;
    int _spsPpsBufferLength;
};

#endif  // HALFWAYLIVE_MEDIAIN_H
