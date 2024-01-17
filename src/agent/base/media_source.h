//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_MEDIA_SOURCE_H
#define HALFWAY_MEDIA_MEDIA_SOURCE_H

#include "../base/media_frame_pipeline.h"
#include <thread>

class MediaSource : public FrameSource {
public:
    MediaSource() = default;
    ~MediaSource() override;

    virtual bool Init();
    virtual bool Start();
    virtual void Stop() { running_ = false; }

protected:
    virtual void ReceiveDataLoop() = 0;

protected:
    bool running_ = true;

private:
    std::unique_ptr<std::thread> workerThread_;
};

#endif // HALFWAY_MEDIA_MEDIA_SOURCE_H
