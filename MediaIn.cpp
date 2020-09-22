//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaIn.h"

MediaIn::MediaIn(const std::string &filename): _filename(filename) {
}

MediaIn::~MediaIn() {
    avformat_close_input(&_avFmtCtx);
}

bool MediaIn::hasAudio() {

    return false;
}

bool MediaIn::hasVideo() {
    return false;
}
