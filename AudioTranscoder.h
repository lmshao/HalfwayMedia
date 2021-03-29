//
// Copyright (c) 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_AUDIOTRANSCODER_H
#define HALFWAYLIVE_AUDIOTRANSCODER_H

#include "MediaFramePipeline.h"

class AudioTranscoder {
  protected:
    virtual void echoAudio() = 0;

    FrameFormat _audioFormat;
};

#endif  // HALFWAYLIVE_AUDIOTRANSCODER_H
