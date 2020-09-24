//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAFILEIN_H
#define HALFWAYLIVE_MEDIAFILEIN_H

#include "MediaIn.h"
#include "Utils.h"

extern "C" {
#include <libavformat/avformat.h>
}

class MediaFileIn : public MediaIn
{
  public:
    explicit MediaFileIn(const std::string &filename);
    ~MediaFileIn() override;

    bool open() override;

  private:
    void checkStream();
};

#endif  // HALFWAYLIVE_MEDIAFILEIN_H
