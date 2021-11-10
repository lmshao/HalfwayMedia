//
// Copyright (c) 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_VIDEOTRANSCODER_H
#define HALFWAYLIVE_VIDEOTRANSCODER_H

#include <map>
#include "MediaFramePipeline.h"
#include "VideoDecoder.h"
#include "VideoEncoder.h"

class VideoTranscoder : public FrameSource, public FrameDestination {
  public:
    VideoTranscoder();
    ~VideoTranscoder();

    void onFrame(const Frame &frame) override;

    bool setInput(int input, FrameFormat format, FrameSource *source);
    void unsetInput(int input);

    bool addOutput(int output, FrameFormat frameFormat, unsigned int width, unsigned int height,
                   unsigned int framerateFPS, unsigned int bitrateKbps, FrameDestination *dest);

    void removeOutput(int output);

    FrameFormat _videoFormat;

  private:
    struct Input {
        FrameSource *source;
        std::shared_ptr<VideoDecoder> decoder;
    };

    struct Output {
        int streamId;
        std::shared_ptr<VideoEncoder> encoder;
    };

    std::map<int, Input> _inputs;
    std::shared_mutex _inputMutex;

    std::map<int, Output> _outputs;
    std::shared_mutex _outputMutex;
};

#endif  // HALFWAYLIVE_VIDEOTRANSCODER_H
