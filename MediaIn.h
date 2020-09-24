//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAIN_H
#define HALFWAYLIVE_MEDIAIN_H

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

class MediaIn
{
  public:
    explicit MediaIn(const std::string& filename);
    virtual ~MediaIn();
    virtual bool open() = 0;
    bool hasAudio() const;
    bool hasVideo() const;

  protected:
    std::string _filename;
    AVFormatContext *_avFmtCtx;

    int _audioStreamNo;
    int _videoStreamNo;

  private:
};

#endif  // HALFWAYLIVE_MEDIAIN_H
