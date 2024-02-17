#include "../session/rtp_pusher_session.h"
#include <cstdio>

int main(int argc, char **argv)
{
    printf("RTP-Pusher, Built at %s on %s.\n", __TIME__, __DATE__);

    auto rtpPusherSession = std::make_unique<RtpPusherSession>();

    rtpPusherSession->SetSourceUrl("../../assets/Luca.mkv");
    rtpPusherSession->SetRtpAddress("192.168.1.100", 1234, 1235);
    if (!rtpPusherSession->Init()) {
        printf("RTP session init failed");
        return 1;
    }

    if (!rtpPusherSession->Start()) {
        printf("RTP session start failed");
        return 1;
    }
}