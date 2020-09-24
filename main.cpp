#include "Utils.h"
#include "MediaFileIn.h"

int main()
{
    logger("Hello Halfway");
//    std::string file = "../assets/Sample.mkv";
    std::string file = "../assets/echo.aac";
    MediaIn *mediaIn = new MediaFileIn(file);
    mediaIn->open();
    if (mediaIn->hasAudio())
        logger("has audio");
    if (mediaIn->hasVideo())
        logger("has video");
    return 0;
}