//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaIn.h"
#include "Utils.h"

MediaIn::MediaIn(const std::string &filename)
  : _url(filename),
    _avFmtCtx(nullptr),
    _audioStreamIndex(-1),
    _videoStreamIndex(-1),
    _needCheckVBS(true),
    _enableVideoExtradata(false)
{
}

MediaIn::~MediaIn()
{
    //    avformat_close_input(&_avFmtCtx);
    logger("~");
}

bool MediaIn::hasAudio() const
{
    return _audioStreamIndex > -1;
}

bool MediaIn::hasVideo() const
{
    return _videoStreamIndex > -1;
}

void MediaIn::waitThread()
{
    _thread.join();
}

void MediaIn::close()
{
    _runing = false;
}
