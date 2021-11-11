//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYLIVE_MEDIAFRAMEPIPELINE_H
#define HALFWAYLIVE_MEDIAFRAMEPIPELINE_H

#include <cstdint>
#include <list>
#include <shared_mutex>
#include <string>

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN = 0,

    FRAME_FORMAT_I420 = 100,
    FRAME_FORMAT_BGRA,

    FRAME_FORMAT_VP8 = 200,
    FRAME_FORMAT_VP9,
    FRAME_FORMAT_H264,
    FRAME_FORMAT_H265,

    FRAME_FORMAT_MSDK = 300,

    FRAME_FORMAT_PCM_48000_2 = 800,
    FRAME_FORMAT_PCM_48000_2_S16,  // s16le pcm
    FRAME_FORMAT_PCM_48000_2_FLT,  // f32le pcm

    FRAME_FORMAT_PCMU = 900,
    FRAME_FORMAT_PCMA,
    FRAME_FORMAT_OPUS,
    FRAME_FORMAT_ISAC16,
    FRAME_FORMAT_ISAC32,
    FRAME_FORMAT_ILBC,
    FRAME_FORMAT_G722_16000_1,
    FRAME_FORMAT_G722_16000_2,

    FRAME_FORMAT_AAC,          // ignore sample rate and channels for decoder, default is 48000_2
    FRAME_FORMAT_AAC_48000_2,  // specify sample rate and channels for encoder

    FRAME_FORMAT_AC3,
    FRAME_FORMAT_NELLYMOSER,

    FRAME_FORMAT_MP3,
};

struct VideoFrameSpecificInfo {
    uint16_t width;
    uint16_t height;
    bool isKeyFrame;
    int framerate;
};

struct AudioFrameSpecificInfo {
    uint8_t isRtpPacket;
    uint32_t nbSamples;
    uint32_t sampleRate;
    uint8_t channels;
};

typedef union MediaSpecInfo {
    VideoFrameSpecificInfo video;
    AudioFrameSpecificInfo audio;
} MediaSpecInfo;

struct Frame {
    FrameFormat format;
    uint8_t *payload;
    uint32_t length;
    uint32_t timeStamp;  // ms ?
    MediaSpecInfo additionalInfo;
};

inline FrameFormat getFormat(const std::string &codec)
{
    if (codec == "vp8") {
        return FRAME_FORMAT_VP8;
    } else if (codec == "h264") {
        return FRAME_FORMAT_H264;
    } else if (codec == "vp9") {
        return FRAME_FORMAT_VP9;
    } else if (codec == "h265") {
        return FRAME_FORMAT_H265;
    } else if (codec == "pcm_48000_2" || codec == "pcm_raw") {
        return FRAME_FORMAT_PCM_48000_2;
    } else if (codec == "pcmu") {
        return FRAME_FORMAT_PCMU;
    } else if (codec == "pcma") {
        return FRAME_FORMAT_PCMA;
    } else if (codec == "isac_16000") {
        return FRAME_FORMAT_ISAC16;
    } else if (codec == "isac_32000") {
        return FRAME_FORMAT_ISAC32;
    } else if (codec == "ilbc") {
        return FRAME_FORMAT_ILBC;
    } else if (codec == "g722_16000_1") {
        return FRAME_FORMAT_G722_16000_1;
    } else if (codec == "g722_16000_2") {
        return FRAME_FORMAT_G722_16000_2;
    } else if (codec == "opus_48000_2") {
        return FRAME_FORMAT_OPUS;
    } else if (codec.compare(0, 3, "aac") == 0) {
        if (codec == "aac_48000_2")
            return FRAME_FORMAT_AAC_48000_2;
        else
            return FRAME_FORMAT_AAC;
    } else if (codec.compare(0, 3, "ac3") == 0) {
        return FRAME_FORMAT_AC3;
    } else if (codec.compare(0, 10, "nellymoser") == 0) {
        return FRAME_FORMAT_NELLYMOSER;
    } else {
        return FRAME_FORMAT_UNKNOWN;
    }
}

inline const char *getFormatStr(const FrameFormat &format)
{
    switch (format) {
        case FRAME_FORMAT_UNKNOWN:
            return "UNKNOWN";
        case FRAME_FORMAT_I420:
            return "I420";
        case FRAME_FORMAT_MSDK:
            return "MSDK";
        case FRAME_FORMAT_VP8:
            return "VP8";
        case FRAME_FORMAT_VP9:
            return "VP9";
        case FRAME_FORMAT_H264:
            return "H264";
        case FRAME_FORMAT_H265:
            return "H265";
        case FRAME_FORMAT_PCM_48000_2:
            return "PCM_48000_2";
        case FRAME_FORMAT_PCMU:
            return "PCMU";
        case FRAME_FORMAT_PCMA:
            return "PCMA";
        case FRAME_FORMAT_OPUS:
            return "OPUS";
        case FRAME_FORMAT_ISAC16:
            return "ISAC16";
        case FRAME_FORMAT_ISAC32:
            return "ISAC32";
        case FRAME_FORMAT_ILBC:
            return "ILBC";
        case FRAME_FORMAT_G722_16000_1:
            return "G722_16000_1";
        case FRAME_FORMAT_G722_16000_2:
            return "G722_16000_2";
        case FRAME_FORMAT_AAC:
            return "AAC";
        case FRAME_FORMAT_AAC_48000_2:
            return "AAC_48000_2";
        case FRAME_FORMAT_AC3:
            return "AC3";
        case FRAME_FORMAT_NELLYMOSER:
            return "NELLYMOSER";
        case FRAME_FORMAT_MP3:
            return "MP3";
        default:
            return "INVALID";
    }
}

inline bool isAudioFrame(const Frame &frame)
{
    return frame.format == FRAME_FORMAT_PCM_48000_2 || frame.format == FRAME_FORMAT_PCMU ||
           frame.format == FRAME_FORMAT_PCMA || frame.format == FRAME_FORMAT_OPUS ||
           frame.format == FRAME_FORMAT_ISAC16 || frame.format == FRAME_FORMAT_ISAC32 ||
           frame.format == FRAME_FORMAT_ILBC || frame.format == FRAME_FORMAT_G722_16000_1 ||
           frame.format == FRAME_FORMAT_G722_16000_2 || frame.format == FRAME_FORMAT_AAC ||
           frame.format == FRAME_FORMAT_AAC_48000_2 || frame.format == FRAME_FORMAT_AC3 ||
           frame.format == FRAME_FORMAT_NELLYMOSER || frame.format == FRAME_FORMAT_MP3;
}

inline bool isVideoFrame(const Frame &frame)
{
    return frame.format == FRAME_FORMAT_I420 || frame.format == FRAME_FORMAT_BGRA ||
           frame.format == FRAME_FORMAT_MSDK || frame.format == FRAME_FORMAT_VP8 || frame.format == FRAME_FORMAT_VP9 ||
           frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265;
}

class FrameDestination;

class FrameSource {
  public:
    FrameSource() = default;
    virtual ~FrameSource();

    void addAudioDestination(FrameDestination *);
    void removeAudioDestination(FrameDestination *);

    void addVideoDestination(FrameDestination *);
    void removeVideoDestination(FrameDestination *);

  protected:
    void deliverFrame(const Frame &);

  private:
    std::list<FrameDestination *> _audioDsts;
    std::shared_mutex _audioDstsMutex;

    std::list<FrameDestination *> _videoDsts;
    std::shared_mutex _videoDstsMutex;
};

class FrameDestination {
  public:
    FrameDestination() : _audioSrc(nullptr), _videoSrc(nullptr) {}
    virtual ~FrameDestination() = default;

    virtual void onFrame(const Frame &) = 0;

    void setAudioSource(FrameSource *);
    void unsetAudioSource();

    void setVideoSource(FrameSource *);
    void unsetVideoSource();

    bool hasAudioSource() { return _audioSrc != nullptr; }
    bool hasVideoSource() { return _videoSrc != nullptr; }

  private:
    FrameSource *_audioSrc;
    std::shared_mutex _audioSrcMutex;
    FrameSource *_videoSrc;
    std::shared_mutex _videoSrcMutex;
};

#endif  // HALFWAYLIVE_MEDIAFRAMEPIPELINE_H
