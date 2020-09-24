//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAOUT_H
#define HALFWAYLIVE_MEDIAOUT_H

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

class MediaOut {
public:
    explicit MediaOut(const std::string &url, bool hasAudio, bool hasVideo);
    virtual ~MediaOut();

protected:
    
    virtual void sendLoop() = 0;

private:
    std::string _url;
    bool _hasAudio;
    bool _hasVideo;
    AVFormatContext *_avFmtCtx;
};


#endif //HALFWAYLIVE_MEDIAOUT_H
