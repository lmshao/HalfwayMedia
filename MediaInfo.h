//
// Created by lmshao on 2021/11/23.
//

#ifndef HALFWAYMEDIA_MEDIAINFO_H
#define HALFWAYMEDIA_MEDIAINFO_H

#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
}

struct VideoMediaInfo {
    std::string format;
    double framerate;
    int width;
    int height;
};

struct AudioMediaInfo {
    std::string format;
    int sampleRate;
    int channels;
};

class MediaInfo {
  public:
    MediaInfo();
    ~MediaInfo();
    explicit MediaInfo(const std::string& filename);
    bool open(const std::string& filename);

    std::string format;
    long duration;

    const VideoMediaInfo& getVideo() const { return _video; }
    const AudioMediaInfo& getAudio() const { return _audio; }

  private:
    VideoMediaInfo _video;
    AudioMediaInfo _audio;
    AVFormatContext* _avFmtCtx;
};

#endif  // HALFWAYMEDIA_MEDIAINFO_H
