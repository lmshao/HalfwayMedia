//
// Created by lmshao on 2021/3/26.
//

#ifndef HALFWAYLIVE_RAWFILEOUT_H
#define HALFWAYLIVE_RAWFILEOUT_H

#include "MediaFramePipeline.h"
#include "Utils.h"

class RawFileOut : public FrameDestination {
  public:
    explicit RawFileOut(const std::string &filename, bool appendMode = false);
    ~RawFileOut();
    void onFrame(const Frame &frame) override;

  private:
    FILE *_file;
};

#endif  // HALFWAYLIVE_RAWFILEOUT_H
