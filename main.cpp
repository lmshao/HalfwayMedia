#include <signal.h>
#include <unistd.h>

#include <string>

#include "Test.h"
#include "Utils.h"

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

// std::shared_ptr<MediaOut> gMediaOut;
// std::shared_ptr<MediaIn> gMediaIn;

void signalHandler(int sig)
{
    if (sig == SIGINT) {
        logger("\nCatch Ctrl+C.");
        //        gMediaIn->close();
        //        gMediaOut->close();
        //        sleep(2);
        exit(SIGINT);
    }
}

int main(int argc, char **argv)
{
    printf("Hello Halfway\n");

    int opt;
    std::string in, out;
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

    //    audio_encoding_pcm_s16le_to_aac();
    //    video_encoding_yuv420_to_h264();
    //    video_encoding_bgra32_to_h264();
    video_converison_bgra32_to_yuv420();
    printf("\n============== main exit! ==============\n");
    return 0;
}