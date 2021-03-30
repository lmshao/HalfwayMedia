//
// Created by lmshao on 2021/3/26.
//

#include "RawFileOut.h"

RawFileOut::RawFileOut(const std::string &filename, bool appendMode)
{
    logger("");
    if (appendMode)
        _fileHandler.open(filename, std::ios::binary | std::ios::app);
    else
        _fileHandler.open(filename, std::ios::binary);
}

RawFileOut::~RawFileOut()
{
    logger("");
    _fileHandler.close();
}

void RawFileOut::onFrame(const Frame &frame)
{
    if (!_fileHandler.is_open())
        return;
    logger("write raw file %d", frame.length);
    DUMP_HEX(frame.payload, 10);
    _fileHandler.write(reinterpret_cast<const char *>(frame.payload), frame.length);
}
