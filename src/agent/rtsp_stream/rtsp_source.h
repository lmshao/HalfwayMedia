//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_RTSP_SOURCE_H
#define HALFWAY_MEDIA_RTSP_SOURCE_H

#include "agent/base/media_source.h"
#include "common/frame.h"
#include "network/tcp_client.h"
#include "network/udp_server.h"
#include "protocol/rtp/rtp_packet.h"
#include "protocol/rtsp/rtsp_response.h"
#include "protocol/rtsp/rtsp_sdp.h"
#include "protocol/rtsp/rtsp_url.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#define RtspClient RtspSource

class RtspSource : public MediaSource, public IClientListener, public IServerListener {
public:
    ~RtspSource();

    template <typename... Args>
    static std::shared_ptr<RtspSource> Create(std::string url)
    {
        return std::shared_ptr<RtspSource>(new RtspSource(std::move(url)));
    }

    // impl MediaSource
    bool Init() override;
    bool Start() override;

    void Stop() override;

    // impl IClientListener of TCP Client
    void OnReceive(std::shared_ptr<DataBuffer> buffer) override;
    void OnClose() override;
    void OnError(const std::string &errorInfo) override;

    // impl IServerListener of UDP Servers
    void OnError(std::shared_ptr<Session> clientSession, const std::string &errorInfo) override;
    void OnClose(std::shared_ptr<Session> clientSession) override {}
    void OnAccept(std::shared_ptr<Session> clientSession) override {}
    void OnReceive(std::shared_ptr<Session> clientSession, std::shared_ptr<DataBuffer> buffer) override;

private:
    RtspSource() = default;
    explicit RtspSource(std::string url) : url_(std::move(url)) {}

    // impl MediaSource
    void ReceiveDataLoop() override;

    bool SendRequestOptions();
    void HandleResponseOptions(RtspResponse &response);

    bool SendRequestDescribe();
    void HandleResponseDescribe(RtspResponse &response);

    bool SendRequestSetup(MediaType type);
    void HandleResponseSetup(RtspResponse &response);

    bool SendRequestPlay();
    void HandleResponsePlay(RtspResponse &response);

    bool StartRtpServers();
    bool InitSinks();

    void InitVideoDepacketizer();
    void InitAudioDepacketizer();

private:
    int cseq_ = 0;
    int timeout_ = 0;
    std::string url_;
    RtspUrl rtspUrl_;
    std::string baseUrl_;
    RtspSdp sdp_;
    std::string session_;
    MediaType setupType_;

    uint16_t remoteVideoRtpPort_ = 0;
    uint16_t remoteVideoRtcpPort_ = 0;
    uint16_t remoteAudioRtpPort_ = 0;
    uint16_t remoteAudioRtcpPort_ = 0;

    std::shared_ptr<UdpServer> videoRtpServer_;
    std::shared_ptr<UdpServer> videoRtcpServer_;
    std::shared_ptr<UdpServer> audioRtpServer_;
    std::shared_ptr<UdpServer> audioRtcpServer_;

    std::shared_ptr<RtpDepacketizer> videoDepacketizer_;
    std::shared_ptr<RtpDepacketizer> audioDepacketizer_;

    std::unique_ptr<TcpClient> rtspConnectionClient_;
    std::unordered_map<int, std::function<void(RtspResponse &)>> responseHandlers_;

    std::shared_ptr<MediaDescription> videoTrack_;
    std::shared_ptr<MediaDescription> audioTrack_;

    std::shared_ptr<Frame> sps_;
    std::shared_ptr<Frame> pps_;
    int videoWidth_ = 0, videoHeight_ = 0;
    int audioSampleRate_ = 0, audioChannels_ = 0;
};

#endif // HALFWAY_MEDIA_RTSP_SOURCE_H