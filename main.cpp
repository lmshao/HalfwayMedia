#include <unistd.h>

#include "AudioDecoder.h"
#include "AudioEncoder.h"
#include "LiveStreamIn.h"
#include "MediaFileIn.h"
#include "MediaFileOut.h"
#include "RawFileIn.h"
#include "RawFileOut.h"
#include "Utils.h"
#include "VideoDecoder.h"
#include "VideoEncoder.h"

enum StreamType { FILE_MODE, RTMP_MODE, RTSP_MODE, UNKNOWN_MODE };

StreamType guessProtocol(std::string &url)
{
    if (url.compare(0, 7, "file://") == 0 || url.compare(0, 1, "/") == 0 || url.compare(0, 1, ".") == 0) {
        return FILE_MODE;
    } else if (url.compare(0, 7, "rtmp://") == 0) {
        return RTMP_MODE;
    } else if (url.compare(0, 7, "rtsp://") == 0) {
        return RTSP_MODE;
    } else {
        return UNKNOWN_MODE;
    }
};

std::shared_ptr<MediaOut> gMediaOut;
std::shared_ptr<MediaIn> gMediaIn;

void signalHandler(int sig)
{
    if (sig == SIGINT) {
        logger("\nCatch Ctrl+C.");
        gMediaIn->close();
        gMediaOut->close();
        sleep(2);
        exit(SIGINT);
    }
}

int main(int argc, char **argv)
{
    printf("Hello Halfway\n");

    //    signal(SIGINT, signalHandler);
    std::string filename = std::to_string(time(nullptr));
    //    std::string in = "../assets/Sample.mkv";
    std::string in = "../Sample/sample-1080p-yuv420p.yuv";
    std::string in_audio = "../Sample/sample-audio-48k-ac2-s16le.pcm";
    //    std::string in = "../assets/Sample.mkv";

    //    std::string in = "rtsp://192.168.188.181:8554/Taylor-720p-1M.ts";
    std::string out = filename + ".mp4";

    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<VideoDecoder> videoDecoder;
    std::shared_ptr<AudioDecoder> audioDecoder;
    std::shared_ptr<VideoEncoder> videoEncoder;
    std::shared_ptr<AudioEncoder> audioEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;
    std::shared_ptr<MediaFileOut> mediaFileOut;

    int opt;
    const char *opts = "i:o:dhv";
    printf("Dump Params\n");
    while ((opt = getopt(argc, argv, opts)) != -1) {
        switch (opt) {
            case 'i':
                printf("-i %s\n", optarg);
                in = optarg;
                break;
            case 'o':
                printf("-o %s\n", optarg);
                out = optarg;
                break;
            case 'd':
                //                decodeFlag = true;
                break;
            case 'h':
                printf("Show Usage: %s -i media_in -o media_out\n", argv[0]);
                return 0;
            case 'v':
                printf("Version: %s %s\n", __DATE__, __TIME__);
                return 0;
            default:
                printf("Show Usage: %s -i media_in -o media_out\n", argv[0]);
                return 1;
        }
    }

    //    switch (guessProtocol(in)) {
    //        case FILE_MODE:
    //        case RTSP_MODE:
    //            mediaIn.reset(new LiveStreamIn(in));
    //            gMediaIn = mediaIn;
    //        default:
    //            break;
    //    }

    //    RawFileInfo info;
    //    info.type = "video";
    //    info.media.video.pix_fmt = AV_PIX_FMT_YUV420P;
    //    info.media.video.height = 1080;
    //    info.media.video.width = 1920;
    //    info.media.video.framerate = 24;
    //    mediaIn.reset(new RawFileIn(in, info));
    //
    //    mediaIn->open();
    //
    //    rawFileOut.reset(new RawFileOut(filename+".h264"));
    //
    //    videoEncoder.reset(new VideoEncoder(FRAME_FORMAT_H264));
    //    videoEncoder->addVideoDestination(rawFileOut.get());
    //    mediaIn->addVideoDestination(videoEncoder.get());
    //
    //    mediaIn->start();
    RawFileInfo info;
    info.type = "audio";
    info.media.audio.sample_fmt = AV_SAMPLE_FMT_S16;
    info.media.audio.channel = 2;
    info.media.audio.sample_rate = 48000;
    mediaIn.reset(new RawFileIn(in_audio, info));
    mediaIn->open();

    audioEncoder.reset(new AudioEncoder(FRAME_FORMAT_AAC_48000_2));

    rawFileOut.reset(new RawFileOut(filename + ".aac"));
    audioEncoder->addAudioDestination(rawFileOut.get());

    //    mediaOut.reset(new MediaFileOut(filename+".mp4", true, false));
    //    audioEncoder->addAudioDestination(mediaOut.get());

    audioEncoder->init();

    mediaIn->addAudioDestination(audioEncoder.get());
    mediaIn->start();

    if (mediaIn != nullptr) mediaIn->waitThread();

    sleep(1);
    if (mediaOut != nullptr) mediaOut->waitThread();
    printf("\n==============\nmain exit!\n==============");
    return 0;
}