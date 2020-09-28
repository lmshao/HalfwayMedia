#include <unistd.h>
#include "Utils.h"
#include "MediaFileIn.h"
#include "MediaFileOut.h"

enum StreamType
{
    FILE_MODE,
    RTMP_MODE,
    UNKNOWN_MODE
};

StreamType guessProtocol(std::string &url)
{
    if (url.compare(0, 7, "file://") == 0 || url.compare(0, 1, "/") == 0 ||
        url.compare(0, 1, ".") == 0) {
        return FILE_MODE;
    } else if (url.compare(0, 7, "rtmp://") == 0) {
        return RTMP_MODE;
    } else {
        return UNKNOWN_MODE;
    }
};

int main(int argc, char **argv)
{
    printf("Hello Halfway\n");
    std::string in = "../assets/Sample.mkv";
    std::string out = "./out.mp4";

    std::shared_ptr<MediaIn> mediaIn;
    std::shared_ptr<MediaOut> mediaOut;

    int opt;
    const char *opts = "i:o:hv";
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
            case 'h':
                printf("Show Usage: %s -i media_in -o media_out\n", argv[0]);
                return 0;
            case 'v':
                printf("Version: %s %s\n", __DATE__, __TIME__);
                break;
            default:
                printf("Show Usage: %s -i media_in -o media_out\n", argv[0]);
                return 1;
        }
    }

    switch (guessProtocol(in)) {
        case FILE_MODE:
            mediaIn.reset(new MediaFileIn(in));
            break;
        default:
            break;
    }

    switch (guessProtocol(out)) {
        case FILE_MODE:
            mediaOut.reset(new MediaFileOut(out, mediaIn->hasAudio(), mediaIn->hasVideo()));
            break;
        default:
            break;
    }

    if (!mediaIn || !mediaOut) {
        logger("param error, in: %s, out: %s", in.c_str(), out.c_str());
        return 0;
    }
    mediaIn->addAudioDestination(mediaOut.get());
    mediaIn->addVideoDestination(mediaOut.get());
    mediaIn->open();

    return 0;
}