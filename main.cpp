#include <unistd.h>
#include "Utils.h"
#include "MediaFileIn.h"
#include "MediaFileOut.h"
#include "LiveStreamIn.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "AudioEncoder.h"
#include "RawFileOut.h"
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
    std::string in = "../assets/Sample.mkv";
    //    std::string in = "rtsp://192.168.188.181:8554/Taylor-720p-1M.ts";
    std::string out = filename + ".mp4";

    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;
    std::shared_ptr<VideoDecoder> videoDecoder;
    std::shared_ptr<AudioDecoder> audioDecoder;
    std::shared_ptr<VideoEncoder> videoEncoder;
    std::shared_ptr<AudioEncoder> audioEncoder;
    std::shared_ptr<RawFileOut> rawFileOut;

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

    switch (guessProtocol(in)) {
        case FILE_MODE:
        case RTSP_MODE:
            mediaIn.reset(new LiveStreamIn(in));
            gMediaIn = mediaIn;
        default:
            break;
    }

    mediaIn->open();

    videoDecoder.reset(new VideoDecoder());
    videoDecoder->init(FRAME_FORMAT_H264);

    mediaIn->addVideoDestination(videoDecoder.get());

    //    rawFileOut.reset(new RawFileOut(filename+".yuv"));
    //    videoDecoder->addVideoDestination(rawFileOut.get());

    videoEncoder.reset(new VideoEncoder(FRAME_FORMAT_H264));
    videoDecoder->addVideoDestination(videoEncoder.get());

    mediaOut.reset(new MediaFileOut(filename + ".mp4", false, true));
    videoEncoder->addVideoDestination(mediaOut.get());

    mediaIn->start();

    if (mediaIn != nullptr)
        mediaIn->waitThread();

    sleep(1);
    if (mediaOut != nullptr)
        mediaOut->waitThread();
    logger("\n==============\nmain exit!\n==============");
    return 0;
}