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
    [[noreturn]] void sendLoop() override;
};


#endif //HALFWAYLIVE_MEDIAFILEOUT_H
