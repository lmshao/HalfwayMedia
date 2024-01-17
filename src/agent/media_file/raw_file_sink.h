//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_RAW_FILE_SINK_H
#define HALFWAY_MEDIA_RAW_FILE_SINK_H

#include "../../common/file_io.h"
#include "../base/media_sink.h"
#include <memory>
#include <utility>

class RawFileSink : public MediaSink {
public:
    ~RawFileSink() override;

    static std::shared_ptr<RawFileSink> Create(std::string file)
    {
        return std::shared_ptr<RawFileSink>(new RawFileSink(std::move(file)));
    }

    // impl FrameSink
    void OnFrame(const std::shared_ptr<Frame> &frame) override;

    // impl MediaSink
    bool Init() override;

private:
    explicit RawFileSink(std::string fileName) : fileName_(std::move(fileName)) {}

private:
    std::string fileName_;
    std::shared_ptr<FileWriter> fileWriter_;
};
#endif // HALFWAY_MEDIA_RAW_FILE_SINK_H