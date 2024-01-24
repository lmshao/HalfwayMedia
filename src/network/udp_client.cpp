//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "udp_client.h"
#include "../common/log.h"
#include "network_common.h"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define RECV_BUFFER_MAX_SIZE 4096

UdpClient::UdpClient(std::string remoteIp, uint16_t remotePort, std::string localIp, uint16_t localPort)
    : remoteIp_(remoteIp), remotePort_(remotePort), localIp_(localIp), localPort_(localPort)
{
}

UdpClient::~UdpClient()
{
    Close();
}

bool UdpClient::Init()
{
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket error: %s", strerror(errno));
        return false;
    }

    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(remotePort_);
    inet_aton(remoteIp_.c_str(), &serverAddr_.sin_addr);

    if (!localIp_.empty() || localPort_ != 0) {
        struct sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(localPort_);
        if (localIp_.empty()) {
            localIp_ = LOCAL_HOST;
        }
        inet_aton(localIp_.c_str(), &localAddr.sin_addr);

        int ret = bind(socket_, (struct sockaddr *)&localAddr, (socklen_t)sizeof(localAddr));
        if (ret != 0) {
            LOGE("bind error: %s", strerror(errno));
            return false;
        }
    }

    return true;
}

void UdpClient::Close()
{
    if (socket_ != INVALID_SOCKET) {
        close(socket_);
        socket_ = INVALID_SOCKET;
    }

    if (clientRecvThread_ && clientRecvThread_->joinable()) {
        clientRecvThread_->join();
        clientRecvThread_.reset();
    }
}

bool UdpClient::Send(const void *data, size_t len)
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    if (!clientListener_.expired() && clientRecvThread_ == nullptr) {
        clientRecvThread_ = std::make_unique<std::thread>(&UdpClient::ReceiveThread, this);
    }

    size_t bytes = sendto(socket_, data, len, 0, (struct sockaddr *)&serverAddr_, (socklen_t)(sizeof(serverAddr_)));
    if (bytes == -1) {
        LOGE("sendto error: %s", strerror(errno));
        return false;
    }

    return true;
}

bool UdpClient::Send(const std::string &str)
{
    return UdpClient::Send(str.c_str(), str.length());
}

bool UdpClient::Send(std::shared_ptr<DataBuffer> data)
{
    return UdpClient::Send((char *)data->Data(), data->Size());
}

void UdpClient::ReceiveThread()
{
    auto dataBuffer = std::make_shared<DataBuffer>(RECV_BUFFER_MAX_SIZE);
    while (socket_ != INVALID_SOCKET) {
        ssize_t bytes = recv(socket_, dataBuffer->Data(), RECV_BUFFER_MAX_SIZE, 0);
        if (bytes > 0) {
            dataBuffer->SetSize(bytes);
            auto listener = clientListener_.lock();
            if (listener) {
                listener->OnReceive(dataBuffer);
            }
        } else if (bytes < 0) {
            LOGE("recv error: %s", strerror(errno));
            continue;
        } else {
            LOGW("disconnected: %s", strerror(errno));
            break;
        }
    }
}