//
// Copyright (c) 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_VIDEOENCODER_H
#define HALFWAYLIVE_VIDEOENCODER_H

#include "MediaFramePipeline.h"

class VideoEncoder : public FrameSource, public FrameDestination {
  public:
    VideoEncoder(FrameFormat format);
    ~VideoEncoder() override;

    void onFrame(const Frame &frame) override;
    int32_t generateStream(uint32_t width, uint32_t height, FrameDestination *dest);
    void degenerateStream(int32_t streamId);

  private:
    std::shared_mutex _mutex;
};

#endif  // HALFWAYLIVE_VIDEOENCODER_H
