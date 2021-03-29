//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAFILEOUT_H
#define HALFWAYLIVE_MEDIAFILEOUT_H

#include "MediaOut.h"

class MediaFileOut : public MediaOut {
  public:
    MediaFileOut(const std::string &url, bool hasAudio, bool hasVideo);

    ~MediaFileOut() override;

  protected:
    bool isAudioFormatSupported(FrameFormat format) override;
    bool isVideoFormatSupported(FrameFormat format) override;

    const char *getFormatName(std::string &url) override;
    bool getHeaderOpt(std::string &url, AVDictionary **options) override;

    uint32_t getKeyFrameInterval() override;
    uint32_t getReconnectCount() override;
};

#endif  // HALFWAYLIVE_MEDIAFILEOUT_H
