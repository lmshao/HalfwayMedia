//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaIn.h"

MediaIn::MediaIn(const std::string &filename)
  : _filename(filename), _avFmtCtx(nullptr), _audioStreamNo(-1), _videoStreamNo(-1)
{
}

MediaIn::~MediaIn()
{
    avformat_close_input(&_avFmtCtx);
}

bool MediaIn::hasAudio() const
{
    return _audioStreamNo > -1;
}

bool MediaIn::hasVideo() const
{
    return _videoStreamNo > -1;
}
