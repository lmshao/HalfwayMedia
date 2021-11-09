//
// Created by lmshao on 2021/3/26.
//

#include "RawFileOut.h"

#include "ADTSHeader.h"

RawFileOut::RawFileOut(const std::string &filename, bool appendMode)
{
    if (appendMode) {
        _file = fopen(filename.c_str(), "w");
    } else {
        _file = fopen(filename.c_str(), "a");
    }
}

RawFileOut::~RawFileOut()
{
    if (_file) {
        fclose(_file);
    }
}

void RawFileOut::onFrame(const Frame &frame)
{
    if (!_file) return;

    logger("write raw file %d", frame.length);
    DUMP_HEX(frame.payload, 10);

    if (frame.format == FRAME_FORMAT_AAC_48000_2) {
        ADTS_Header header(48000, 2, frame.length + 7);
        fwrite(&header, 1, sizeof(ADTS_Header), _file);
    }

    if (fwrite(frame.payload, 1, frame.length, _file) != frame.length){
        perror("write error:");
        return;
    }
}
