#include "../session/rtsp_client_session.h"
#include <cstdio>
#include <memory>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("RTSP-Client, Built at %s on %s.\n", __TIME__, __DATE__);

    auto rtspRecoderSession = std::make_unique<RtspClientSession>();

    // rtspRecoderSession->SetSourceUrl("rtsp://192.168.1.83:8554/Luca-30s-720p.mkv");
    rtspRecoderSession->SetSourceUrl("rtsp://192.168.1.20:8554/Luca-30s-720p.mkv");
    rtspRecoderSession->SetRecorderFileName("recorder");

    if (!rtspRecoderSession->Init()) {
        printf("RTP session init failed");
        return 1;
    }

    if (!rtspRecoderSession->Start()) {
        printf("RTP session start failed");
        return 1;
    }

    printf("RTP session start ok");

    while (true) {
        sleep(10);
    }

    return 0;
}
