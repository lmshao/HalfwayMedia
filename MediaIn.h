//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAIN_H
#define HALFWAYLIVE_MEDIAIN_H


#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

class MediaIn {
public:
    MediaIn(const std::string &filename);
    virtual ~MediaIn();
    virtual bool open() = 0;

protected:
    std::string _filename;
    AVFormatContext *_avFmtCtx;

    bool hasAudio();
    bool hasVideo();
private:

};


#endif //HALFWAYLIVE_MEDIAIN_H
