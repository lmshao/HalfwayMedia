#include "rtsp_source.h"
#include "agent/base/event_definition.h"
#include "common/frame.h"
#include "common/log.h"
#include "common/utils.h"
#include "protocol/rtsp/rtsp_request.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

const char *RTSP_USER_AGENT = "HalfwatMedia/2.0";
const char *MIME_SDP = "application/sdp";

RtspSource::~RtspSource()
{
    LOGD("destructor");
    Stop();
}

bool RtspSource::Init()
{
    UrlType type = DetectUrlType(url_);
    if (type != TYPE_RTSP) {
        LOGE("url invalid: %s", url_.c_str());
        return false;
    }

    if (!rtspUrl_.Parse(url_)) {
        LOGE("url invalid: %s", url_.c_str());
        return false;
    }

    rtspConnectionClient_ = std::make_unique<TcpClient>(rtspUrl_.GetHostName(), rtspUrl_.GetPort());
    rtspConnectionClient_->SetListener(std::dynamic_pointer_cast<RtspSource>(shared_from_this()));

    if (!rtspConnectionClient_->Init()) {
        LOGE("TcpClient init error");
        return false;
    }

    if (!rtspConnectionClient_->Connect()) {
        LOGE("TcpClient Connect error");
        return false;
    }

    return true;
}

bool RtspSource::Start()
{
    if (!SendRequestOptions()) {
        LOGE("Send OPTIONS error");
        return false;
    }

    return MediaSource::Start();
}

void RtspSource::Stop()
{
    if (videoRtpServer_) {
        videoRtpServer_->Stop();
        videoRtpServer_.reset();
    }

    if (videoRtcpServer_) {
        videoRtcpServer_->Stop();
        videoRtcpServer_.reset();
    }

    if (audioRtpServer_) {
        audioRtpServer_->Stop();
        audioRtpServer_.reset();
    }

    if (audioRtcpServer_) {
        audioRtcpServer_->Stop();
        audioRtcpServer_.reset();
    }
}

void RtspSource::ReceiveDataLoop()
{
    LOGD("enter");
}

void RtspSource::OnReceive(std::shared_ptr<DataBuffer> buffer)
{
    LOGD("%zu bytes, content:\n%s", buffer->Size(), buffer->ToString().c_str());

    RtspResponse response;
    bool res = response.Parse(buffer->ToString());
    if (res == false) {
        LOGE("parse response failed");
        return;
    }

    auto handler = responseHandlers_.find(response.GetCSeq());
    if (handler == responseHandlers_.end()) {
        LOGE("Can't find handler for CSeq(%d)", response.GetCSeq());
    }

    if (response.GetStatusCode() == StatusCode::OK) {
        handler->second(response);
    } else {
        LOGE("RTSP error: (%d) %s", response.GetStatusCode(), response.GetStatusMessage().c_str());
    }
    responseHandlers_.erase(response.GetCSeq());
}

void RtspSource::OnClose()
{
    LOGW("Server close connection");
    Stop();
}

void RtspSource::OnError(const std::string &errorInfo)
{
    LOGW("Server error %s", errorInfo.c_str());
    Stop();
}

void RtspSource::OnError(std::shared_ptr<Session> clientSession, const std::string &errorInfo)
{
    if (clientSession->fd == 0) {
        LOGE("Unexpected situation");
        return;
    }

    if (videoRtpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Video Rtp connection error: %s", errorInfo.c_str());
    } else if (videoRtcpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Video Rtcp connection error: %s", errorInfo.c_str());
    } else if (audioRtpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Audio Rtp connection error: %s", errorInfo.c_str());
    } else if (audioRtcpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Audio Rtcp connection error: %s", errorInfo.c_str());
    } else {
        LOGE("Unexpected situation");
    }
}

void RtspSource::OnReceive(std::shared_ptr<Session> clientSession, std::shared_ptr<DataBuffer> buffer)
{
    if (clientSession->fd == 0) {
        LOGE("Unexpected situation");
        return;
    }

    if (videoRtpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Video Rtp recv %zu bytes", buffer->Size());

        if (videoDepacketizer_) {
            videoDepacketizer_->Depacketize(buffer);
        }
    } else if (videoRtcpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Video Rtcp recv %zu bytes", buffer->Size());

    } else if (audioRtpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Audio Rtp recv %zu bytes", buffer->Size());

        if (audioDepacketizer_) {
            audioDepacketizer_->Depacketize(buffer);
        }
    } else if (audioRtcpServer_->GetSocketFd() == clientSession->fd) {
        LOGD("Audio Rtcp recv %zu bytes", buffer->Size());

    } else {
        LOGE("Unexpected situation");
    }
}

bool RtspSource::SendRequestOptions()
{
    RtspRequestOptions request(url_);
    request.SetCSeq(++cseq_);
    request.SetUserAgent(RTSP_USER_AGENT);
    auto reqStr = request.Stringify();

    responseHandlers_.emplace(cseq_, [this](RtspResponse &response) { HandleResponseOptions(response); });
    LOGD("Send:\n%s", reqStr.c_str());
    if (rtspConnectionClient_) {
        return rtspConnectionClient_->Send(reqStr);
    }

    return false;
}

void RtspSource::HandleResponseOptions(RtspResponse &response)
{
    RtspResponseOptions options(response);
    auto publics = options.GetPublic();
    for (auto &p : publics) {
        LOGD("Public item: %s", p.c_str());
    }

    SendRequestDescribe();
}

bool RtspSource::SendRequestDescribe()
{
    RtspRequestDescribe request(url_);
    request.SetCSeq(++cseq_);
    request.SetUserAgent(RTSP_USER_AGENT);
    request.SetAccept(MIME_SDP);
    auto reqStr = request.Stringify();

    responseHandlers_.emplace(cseq_, [this](RtspResponse &response) { HandleResponseDescribe(response); });
    LOGD("Send:\n%s", reqStr.c_str());
    if (rtspConnectionClient_) {
        return rtspConnectionClient_->Send(reqStr);
    }

    return false;
}

void RtspSource::HandleResponseDescribe(RtspResponse &response)
{
    RtspResponseDescribe describe(response);
    baseUrl_ = describe.GetContentBaseUrl();
    if (describe.GetContentType() != MIME_SDP) {
        LOGE("Expect to receive sdp");
        return;
    }

    std::string sdp = describe.GetMessageBody();
    int contentLength = describe.GetContentLength();
    if (sdp.size() != contentLength) {
        LOGW("Content-Length: %d, but the actual size of the sdp is: %ld", contentLength, sdp.size());
    }

    if (!sdp_.Parse(sdp)) {
        LOGE("Parse sdp failed");
        return;
    }

    videoTrack_ = sdp_.GetVideoTrack();
    audioTrack_ = sdp_.GetAudioTrack();

    if (videoTrack_) {
        InitVideoDepacketizer();
    }

    if (audioTrack_) {
        InitAudioDepacketizer();
    }

    if (videoTrack_) {
        SendRequestSetup(VIDEO);
    } else if (audioTrack_) {
        SendRequestSetup(AUDIO);
    } else {
        LOGE("Can't find video & audio track from sdp");
    }
}

bool RtspSource::SendRequestSetup(MediaType type)
{
    RtspRequestSetup request;
    if (type == VIDEO) {
        std::string videoTrackUrl = videoTrack_->GetTrackId();
        if (videoTrackUrl.find(baseUrl_) != std::string::npos) {
            request.SetUrl(videoTrackUrl);
        } else {
            request.SetUrl(baseUrl_ + "" + videoTrackUrl);
        }

        uint16_t videoRtpPort = UdpServer::GetIdlePortPair();
        videoRtpServer_ = UdpServer::Create(videoRtpPort);
        videoRtcpServer_ = UdpServer::Create(videoRtpPort + 1);
        videoRtpServer_->SetListener(std::dynamic_pointer_cast<RtspSource>(shared_from_this()));
        videoRtcpServer_->SetListener(std::dynamic_pointer_cast<RtspSource>(shared_from_this()));

        request.SetTransport(videoRtpPort, videoRtpPort + 1); // SET VIDEO RTP SERVER PORT
        setupType_ = VIDEO;

    } else if (type == AUDIO) {
        std::string audioTrackUrl = audioTrack_->GetTrackId();
        if (audioTrackUrl.find(baseUrl_) != std::string::npos) {
            request.SetUrl(audioTrackUrl);
        } else {
            request.SetUrl(baseUrl_ + "" + audioTrackUrl);
        }

        uint16_t audioRtpPort = UdpServer::GetIdlePortPair();
        audioRtpServer_ = UdpServer::Create(audioRtpPort);
        audioRtcpServer_ = UdpServer::Create(audioRtpPort + 1);
        audioRtpServer_->SetListener(std::dynamic_pointer_cast<RtspSource>(shared_from_this()));
        audioRtcpServer_->SetListener(std::dynamic_pointer_cast<RtspSource>(shared_from_this()));

        request.SetTransport(audioRtpPort, audioRtpPort + 1); // SET AUDIO RTP SERVER PORT
        setupType_ = AUDIO;
        LOGD("send SETUP");
    }

    request.SetCSeq(++cseq_);
    request.SetUserAgent(RTSP_USER_AGENT);

    if (!session_.empty()) {
        request.SetSession(session_);
    }

    auto reqStr = request.Stringify();

    responseHandlers_.emplace(cseq_, [this](RtspResponse &response) { HandleResponseSetup(response); });
    LOGD("Send:\n%s", reqStr.c_str());
    if (rtspConnectionClient_) {
        return rtspConnectionClient_->Send(reqStr);
    }

    return false;
}

void RtspSource::HandleResponseSetup(RtspResponse &response)
{
    RtspResponseSetup setup(response);

    session_ = setup.GetSession();
    timeout_ = setup.GetTimeout();

    std::pair<uint16_t, uint16_t> sport = setup.GetServerPort();
    LOGD("server_port: %d/rtp, %d/rtcp", sport.first, sport.second);

    if (setupType_ == VIDEO) {
        remoteVideoRtpPort_ = sport.first;
        remoteAudioRtcpPort_ = sport.second;

        if (audioTrack_) {
            SendRequestSetup(AUDIO);
            return;
        }
    } else if (setupType_ == AUDIO) {
        remoteAudioRtpPort_ = sport.first;
        remoteAudioRtcpPort_ = sport.second;
    }

    SendRequestPlay();
}

bool RtspSource::SendRequestPlay()
{
    if (!StartRtpServers()) {
        LOGE("Start rtp server failed.");
        return false;
    }

    if (!InitSinks()) {
        LOGE("Init sinks failed.");
        return false;
    }

    RtspRequestPlay request(baseUrl_);
    request.SetCSeq(++cseq_);
    request.SetUserAgent(RTSP_USER_AGENT);
    if (!session_.empty()) {
        request.SetSession(session_);
    }

    auto reqStr = request.Stringify();

    responseHandlers_.emplace(cseq_, [this](RtspResponse &response) { HandleResponsePlay(response); });
    LOGD("Send:\n%s", reqStr.c_str());
    if (rtspConnectionClient_) {
        return rtspConnectionClient_->Send(reqStr);
    }

    return false;
}

void RtspSource::HandleResponsePlay(RtspResponse &response)
{
    RtspResponsePlay play(response);

    std::string info = play.GetRtpInfo();
    LOGD("RTP-Info: %s", info.c_str());
}

bool RtspSource::StartRtpServers()
{

    if (videoRtpServer_) {
        if (!videoRtpServer_->Init() || !videoRtpServer_->Start()) {
            LOGE("Start video RTP server failed.");
            return false;
        }
    }

    if (videoRtcpServer_) {
        if (!videoRtcpServer_->Init() || !videoRtcpServer_->Start()) {
            LOGE("Start video RTP server failed.");
            return false;
        }
    }

    if (audioRtpServer_) {
        if (!audioRtpServer_->Init() || !audioRtpServer_->Start()) {
            LOGE("Start video RTP server failed.");
            return false;
        }
    }

    if (audioRtcpServer_) {
        if (!audioRtcpServer_->Init() || !audioRtcpServer_->Start()) {
            LOGE("Start video RTP server failed.");
            return false;
        }
    }

    return true;
}

bool RtspSource::InitSinks()
{
    AgentEvent eventSetParams{EVENT_SINK_SET_PARAMETERS};
    MediaParameters mediaParams{nullptr, nullptr};
    eventSetParams.params = &mediaParams;
    VideoFrameInfo videoInfo;
    AudioFrameInfo audioInfo;

    if (videoTrack_) {
        LOGW("%d x %d", videoWidth_, videoHeight_);
        videoInfo.width = videoWidth_;
        videoInfo.height = videoHeight_;
        mediaParams.video = &videoInfo;
    }

    if (audioTrack_) {
        audioInfo.channels = audioChannels_;
        audioInfo.sampleRate = audioSampleRate_;
        audioInfo.nbSamples = 1024;
        mediaParams.audio = &audioInfo;
    }

    if (!NotifySink(eventSetParams)) {
        LOGE("init sink failed");
        return false;
    }

    AgentEvent eventStart{EVENT_SINK_INIT, nullptr};

    if (!NotifySink(eventStart)) {
        LOGE("start sink failed");
        return false;
    }

    return true;
}

void RtspSource::InitVideoDepacketizer()
{
    if (!videoTrack_) {
        return;
    }

    auto reso = videoTrack_->GetVideoSize();
    videoWidth_ = reso.first;
    videoHeight_ = reso.second;

    auto sps = videoTrack_->GetVideoSps();
    auto pps = videoTrack_->GetVideoPps();
    if (sps) {
        sps_ = std::make_shared<Frame>();
        sps_->Assign(sps->Data(), sps->Size());
        sps_->format = FRAME_FORMAT_H264;
    }

    if (pps) {
        pps_ = std::make_shared<Frame>();
        pps_->Assign(pps->Data(), pps->Size());
        pps_->format = FRAME_FORMAT_H264;
    }

    int pt, clockCycle;
    std::string format;
    if (videoTrack_->GetVideoConfig(pt, format, clockCycle)) {
        if (format == "H264" || format == "h264") {
            videoDepacketizer_ = RtpDepacketizer::Create(FRAME_FORMAT_H264);
            if (!videoDepacketizer_) {
                LOGE("Create RtpDepacketizer for aac failed");
                return;
            }

            videoDepacketizer_->SetCallback([this](std::shared_ptr<Frame> frame) {
                // if (frame->videoInfo.isKeyFrame && sps_ && pps_) {
                //     sps_->timestamp = frame->timestamp;
                //     DeliverFrame(sps_);
                //     pps_->timestamp = frame->timestamp;
                //     DeliverFrame(pps_);
                // }

                frame->videoInfo.width = videoWidth_;
                frame->videoInfo.height = videoHeight_;
                DeliverFrame(frame);
            });
        } else {
            LOGE("Unsupported Video format %s", format.c_str());
            return;
        }
    }
}

void RtspSource::InitAudioDepacketizer()
{
    if (!audioTrack_) {
        return;
    }

    int pt;
    std::string format;
    if (audioTrack_->GetAudioConfig(pt, format, audioSampleRate_, audioChannels_)) {
        if (format == "MPEG4-GENERIC" || format == "mpeg4-generic") {
            audioDepacketizer_ = RtpDepacketizer::Create(FRAME_FORMAT_AAC);
            if (!audioDepacketizer_) {
                LOGE("Create RtpDepacketizer for aac failed");
                return;
            }

            AudioFrameInfo info{(uint8_t)audioChannels_, 1024, (uint32_t)audioSampleRate_};
            audioDepacketizer_->SetExtraData(&info);
            audioDepacketizer_->SetCallback([this](std::shared_ptr<Frame> frame) { DeliverFrame(frame); });
        } else {
            LOGE("Unsupported Audio format %s", format.c_str());
            return;
        }
    }
}