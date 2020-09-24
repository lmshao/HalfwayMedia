//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileIn.h"

MediaFileIn::MediaFileIn(const std::string &filename) : MediaIn(filename) {}

MediaFileIn::~MediaFileIn() = default;

bool MediaFileIn::open()
{
    int ret = avformat_open_input(&_avFmtCtx, _filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        logger("Failed to open input file '%s': %s", _filename.c_str(), ff_err2str(ret));
        return false;
    }

    logger("nb_stream=%d", _avFmtCtx->nb_streams);
    ret = avformat_find_stream_info(_avFmtCtx, nullptr);
    if (ret < 0) {
        logger("Failed to retrieve input stream information: %s", ff_err2str(ret));
        return false;
    }
    av_dump_format(_avFmtCtx, 0, _filename.c_str(), 0);

    checkStream();
    return true;
}

void MediaFileIn::checkStream()
{
    if (!_avFmtCtx)
        return;
    for (int i = 0; i < _avFmtCtx->nb_streams; ++i) {
        AVCodecParameters *codecPar = _avFmtCtx->streams[i]->codecpar;
        switch (codecPar->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                if (_audioStreamNo == -1)
                    _audioStreamNo = i;
                break;
            case AVMEDIA_TYPE_VIDEO:
                if (_videoStreamNo == -1)
                    _videoStreamNo = i;
                break;
            default:
                break;
        }
    }
}
