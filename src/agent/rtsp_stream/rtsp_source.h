//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_RTSP_SOURCE_H
#define HALFWAY_MEDIA_RTSP_SOURCE_H

#include "../../common/frame.h"
#include "../../network/tcp_client.h"
#include "../../network/udp_server.h"
#include "../../protocol/rtsp/rtsp_response.h"
#include "../../protocol/rtsp/rtsp_sdp.h"
#include "../../protocol/rtsp/rtsp_url.h"
#include "../base/media_source.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#define RtspClient RtspSource

class RtspSource : public MediaSource, public IClientListener {
public:
    ~RtspSource() = default;

    template <typename... Args>
    static std::shared_ptr<RtspSource> Create(std::string url)
    {
        return std::shared_ptr<RtspSource>(new RtspSource(std::move(url)));
    }

    // impl MediaSource
    bool Init() override;
    bool Start() override;

    // impl IClientListener of TCP Client
    void OnReceive(std::shared_ptr<DataBuffer> buffer) override;
    void OnClose() override;
    void OnError(const std::string &errorInfo) override;

private:
    RtspSource() = default;
    explicit RtspSource(std::string url) : url_(std::move(url)) {}

    // impl MediaSource
    void ReceiveDataLoop() override;

    bool SendRtspOptions();
    void HandleResponseOptions(RtspResponse &response);
    bool SendRtspDescribe();
    void HandleResponseDescribe(RtspResponse &response);
    bool SendRtspSetup(MediaType type);
    void HandleResponseSetup(RtspResponse &response);
    bool SendRtspPlay();
    void HandleResponsePlay(RtspResponse &response);

private:
    int cseq_ = 0;
    std::string url_;
    RtspUrl rtspUrl_;
    std::string baseUrl_;
    RtspSdp sdp_;
    std::string session_;
    MediaType setupType_;

    std::shared_ptr<UdpServer> videoRtpServer_;
    std::shared_ptr<UdpServer> videoRtcpServer_;
    std::shared_ptr<UdpServer> audioRtpServer_;
    std::shared_ptr<UdpServer> audioRtcpServer_;
    std::unique_ptr<TcpClient> rtspConnectionClient_;
    std::unordered_map<int, std::function<void(RtspResponse &)>> responseHandlers_;

    std::shared_ptr<MediaDescription> videoTrack_;
    std::shared_ptr<MediaDescription> audioTrack_;
};

#endif // HALFWAY_MEDIA_RTSP_SOURCE_H