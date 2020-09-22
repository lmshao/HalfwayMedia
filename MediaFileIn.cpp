//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileIn.h"

bool MediaFileIn::open() {
    if (avformat_open_input(&_avFmtCtx, _filename.c_str(), nullptr, nullptr) < 0) {
        logger("Failed to open input file '%s'", _filename.c_str());
        return false;
    }

    logger("nb_stream=%d", _avFmtCtx->nb_streams);
    if (avformat_find_stream_info(_avFmtCtx, nullptr) < 0) {
        logger("Failed to retrieve input stream information");
        return false;
    }
    av_dump_format(_avFmtCtx, 0, _filename.c_str(), 0);
    return true;
}

MediaFileIn::MediaFileIn(const std::string &filename) : MediaIn(filename) {

}

MediaFileIn::~MediaFileIn() = default;
