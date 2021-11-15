//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "Test.h"

#include <unistd.h>

#include "AudioEncoder.h"
#include "ImageConversion.h"
#include "MediaOut.h"
#include "RawFileIn.h"
#include "RawFileOut.h"
#include "Resampling.h"
#include "VideoEncoder.h"

std::string ts = std::to_string(time(nullptr));

int audio_encoding_pcm_s16le_to_aac()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<AudioEncoder> audioEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;

    RawFileInfo info;
    info.type = "audio";
    info.media.audio.sample_fmt = "s16le";
    info.media.audio.channel = 2;
    info.media.audio.sample_rate = 48000;
    mediaIn.reset(new RawFileIn("../Sample/sample-audio-48k-ac2-s16le.pcm", info));
    if (!mediaIn->open())
        return false;

    audioEncoder.reset(new AudioEncoder(FRAME_FORMAT_AAC_48000_2));

    rawFileOut.reset(new RawFileOut(ts + ".aac"));
    audioEncoder->addAudioDestination(rawFileOut.get());

    //    mediaOut.reset(new MediaFileOut(filename+".mp4", true, false));
    //    audioEncoder->addAudioDestination(mediaOut.get());

    audioEncoder->init();

    mediaIn->addAudioDestination(audioEncoder.get());
    mediaIn->start();

    if (mediaIn != nullptr)
        mediaIn->waitThread();

    audioEncoder->flush();

    sleep(1);
    if (mediaOut != nullptr)
        mediaOut->waitThread();
    return 0;
}

int video_encoding_yuv420_to_h264()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<VideoEncoder> videoEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;

    RawFileInfo info;
    info.type = "video";
    info.media.video.pix_fmt = "yuv420";
    info.media.video.height = 1080;
    info.media.video.width = 1920;
    info.media.video.framerate = 24;
    mediaIn.reset(new RawFileIn("../Sample/sample-1080p-yuv420p.yuv", info));

    if (!mediaIn->open())
        return -1;

    rawFileOut.reset(new RawFileOut(ts + ".h264"));

    videoEncoder.reset(new VideoEncoder(FRAME_FORMAT_H264));
    videoEncoder->addVideoDestination(rawFileOut.get());

    mediaIn->addVideoDestination(videoEncoder.get());

    mediaIn->start();

    if (mediaIn != nullptr) {
        mediaIn->waitThread();
    }

    sleep(1);
    if (mediaOut != nullptr) {
        mediaOut->waitThread();
    }
    return 0;
}

int video_encoding_bgra32_to_h264()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<VideoEncoder> videoEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;

    RawFileInfo info;
    info.type = "video";
    info.media.video.pix_fmt = "bgra";
    info.media.video.height = 1080;
    info.media.video.width = 1920;
    info.media.video.framerate = 24;
    mediaIn.reset(new RawFileIn("../Sample/sample-1080p-bgra.rgb", info));

    if (!mediaIn->open())
        return -1;

    rawFileOut.reset(new RawFileOut(ts + ".h264"));

    videoEncoder.reset(new VideoEncoder(FRAME_FORMAT_H264));
    videoEncoder->addVideoDestination(rawFileOut.get());

    mediaIn->addVideoDestination(videoEncoder.get());

    mediaIn->start();

    if (mediaIn != nullptr) {
        mediaIn->waitThread();
    }

    sleep(1);
    if (mediaOut != nullptr) {
        mediaOut->waitThread();
    }
    return 0;
}

int video_converison_bgra32_to_yuv420()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<ImageConversion> imageConversion;
    std::shared_ptr<RawFileOut> rawFileOut;

    RawFileInfo info;
    info.type = "video";
    info.media.video.pix_fmt = "bgra";
    info.media.video.height = 1080;
    info.media.video.width = 1920;
    info.media.video.framerate = 24;
    mediaIn.reset(new RawFileIn("../Sample/sample-1080p-bgra.rgb", info));

    if (!mediaIn->open())
        return -1;

    rawFileOut.reset(new RawFileOut(ts + ".yuv"));

    imageConversion.reset(new ImageConversion());
    imageConversion->addVideoDestination(rawFileOut.get());

    mediaIn->addVideoDestination(imageConversion.get());
    mediaIn->start();

    if (mediaIn != nullptr) {
        mediaIn->waitThread();
    }

    sleep(10);

    return 0;
}

int audio_conversion_f32le_to_s16le()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<AudioEncoder> audioEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;
    std::shared_ptr<Resampling> resampler;

    RawFileInfo info;
    info.type = "audio";
    info.media.audio.sample_fmt = "f32le";
    info.media.audio.channel = 2;
    info.media.audio.sample_rate = 48000;
    mediaIn.reset(new RawFileIn("../Sample/sample-audio-48k-ac2-f32le.pcm", info));
    if (!mediaIn->open())
        return false;

    resampler.reset(new Resampling());

    rawFileOut.reset(new RawFileOut(ts + ".pcm"));
    resampler->addAudioDestination(rawFileOut.get());

    mediaIn->addAudioDestination(resampler.get());
    mediaIn->start();

    if (mediaIn != nullptr)
        mediaIn->waitThread();

    sleep(1);
    return 0;
}

int audio_encoding_pcm_f32le_to_aac()
{
    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<AudioEncoder> audioEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;

    RawFileInfo info;
    info.type = "audio";
    info.media.audio.sample_fmt = "f32le";
    info.media.audio.channel = 2;
    info.media.audio.sample_rate = 48000;
    mediaIn.reset(new RawFileIn("../Sample/sample-audio-48k-ac2-f32le.pcm", info));

    if (!mediaIn->open())
        return false;

    audioEncoder.reset(new AudioEncoder(FRAME_FORMAT_AAC_48000_2));
    mediaIn->addAudioDestination(audioEncoder.get());

    rawFileOut.reset(new RawFileOut(ts + ".aac"));
    audioEncoder->addAudioDestination(rawFileOut.get());
    audioEncoder->init();

    mediaIn->start();

    if (mediaIn != nullptr)
        mediaIn->waitThread();

    audioEncoder->flush();
    sleep(1);
    return 0;
}
