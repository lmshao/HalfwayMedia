//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaOut.h"
#include "Utils.h"

MediaOut::MediaOut(const std::string &url, bool hasAudio, bool hasVideo)
  : _url(url), _hasAudio(hasAudio), _hasVideo(hasVideo), _avFmtCtx(nullptr)
{
    logger("url %s, audio %d, video %d", _url.c_str(), _hasAudio, _hasVideo);
}

MediaOut::~MediaOut() = default;
