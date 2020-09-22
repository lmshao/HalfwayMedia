#include "Utils.h"
#include "MediaFileIn.h"

int main()
{
    logger("Hello Halfway");
    MediaIn *mediaIn = new MediaFileIn("../assets/Sample.mkv");
    mediaIn->open();
    return 0;
}