#include "common/log.h"
#include "session/rtp_pusher_session.h"
#include "session/rtsp_recorder_session.h"
#include <iostream>
#include <memory>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("Hello Halfway, Built at %s on %s.\n", __TIME__, __DATE__);

    // test rtp pusher
    // auto rtpPusherSession = std::make_unique<RtpPusherSession>();

    // rtpPusherSession->SetSourceUrl("../assets/Luca.mkv");
    // rtpPusherSession->SetRtpAddress("192.168.1.100", 1234, 1235);

    // if (!rtpPusherSession->Init()) {
    //     LOGE("RTP session init failed");
    //     return 1;
    // }

    // if (!rtpPusherSession->Start()) {
    //     LOGE("RTP session start failed");
    //     return 1;
    // }

    // test rtp pusher
    auto rtspRecoderSession = std::make_unique<RtspRecoderSession>();

    rtspRecoderSession->SetSourceUrl("rtsp://192.168.1.20:8554/Luca-30s-720p.mkv");
    rtspRecoderSession->SetRecorderFileName("recorder.h264");

    if (!rtspRecoderSession->Init()) {
        LOGE("RTP session init failed");
        return 1;
    }

    if (!rtspRecoderSession->Start()) {
        LOGE("RTP session start failed");
        return 1;
    }

    LOGD("RTP session start ok");

    while (true) {
        sleep(10);
    }

    return 0;
}
