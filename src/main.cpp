#include "common/log.h"
#include "session/rtp_pusher_session.h"
#include <iostream>
#include <memory>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("Hello Halfway, Built at %s on %s.\n", __TIME__, __DATE__);

    auto rtpPusherSession = std::make_unique<RtpPusherSession>();

    rtpPusherSession->SetSourceUrl("../assets/Luca.mkv");
    rtpPusherSession->SetRtpAddress("192.168.1.100", 1234, 1235);

    if (!rtpPusherSession->Init()) {
        LOGE("RTP session init failed");
        return 1;
    }

    if (!rtpPusherSession->Start()) {
        LOGE("RTP session start failed");
        return 1;
    }

    LOGD("RTP session start ok");

    while (true) {
        sleep(10);
    }

    return 0;
}
