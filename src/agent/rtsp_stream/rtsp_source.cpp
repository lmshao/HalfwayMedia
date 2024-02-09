#include "rtsp_source.h"
#include "../../common/log.h"
#include "../../common/utils.h"
#include "../../protocol/rtsp/rtsp_request.h"
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>

const char *RTSP_USER_AGENT = "HalfwatMedia/2.0";
const char *MIME_SDP = "application/sdp";

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
    if (!SendRtspOptions()) {
        LOGE("Send OPTIONS error");
        return false;
    }

    return MediaSource::Start();
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
}

void RtspSource::OnError(const std::string &errorInfo)
{
    LOGW("Server error %s", errorInfo.c_str());
}

bool RtspSource::SendRtspOptions()
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

    SendRtspDescribe();
}

bool RtspSource::SendRtspDescribe()
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
        SendRtspSetup(VIDEO);
    } else if (audioTrack_) {
        SendRtspSetup(AUDIO);
    } else {
        LOGE("Can't find video & audio track from sdp");
    }
}

bool RtspSource::SendRtspSetup(MediaType type)
{
    RtspRequestSetup request;
    if (type == VIDEO) {
        request.SetUrl(baseUrl_ + "" + videoTrack_->GetTrackId());

        uint16_t videoRtpPort = UdpServer::GetIdlePortPair();
        videoRtpServer_ = UdpServer::Create(videoRtpPort);
        videoRtcpServer_ = UdpServer::Create(videoRtpPort + 1);

        request.SetTransport(videoRtpPort, videoRtpPort + 1); // SET VIDEO RTP SERVER PORT
        setupType_ = VIDEO;

    } else if (type == AUDIO) {
        request.SetUrl(baseUrl_ + "" + audioTrack_->GetTrackId());

        uint16_t audioRtpPort = UdpServer::GetIdlePortPair();
        audioRtpServer_ = UdpServer::Create(audioRtpPort);
        audioRtcpServer_ = UdpServer::Create(audioRtpPort + 1);

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

    // Response: RTSP/1.0 200 OK\r\n
    // CSeq: 5\r\n
    // Date: Tue, Feb 06 2024 07:42:14 GMT\r\n
    // Transport:
    // RTP/AVP;unicast;destination=192.168.1.100;source=192.168.1.101;client_port=62874-62875;server_port=6972-6973
    // Session: C1D99D46;timeout=65

    if (setupType_ == VIDEO) {
        // TODO: handle video setup response

        if (audioTrack_) {

            SendRtspSetup(AUDIO);
            return;
        }
    } else if (setupType_ == AUDIO) {
        // TODO: handle audio setup response
    }

    SendRtspPlay();
}

bool RtspSource::SendRtspPlay()
{
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

void RtspSource::HandleResponsePlay(RtspResponse &response) {}