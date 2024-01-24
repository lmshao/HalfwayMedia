//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "tcp_client.h"
#include "../common/utils.h"
#include "event_processor.h"
#include "network_common.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const int RECV_BUFFER_MAX_SIZE = 4096;

TcpClient::TcpClient(std::string remoteIp, uint16_t remotePort, std::string localIp, uint16_t localPort)
    : remoteIp_(remoteIp), remotePort_(remotePort), localIp_(localIp), localPort_(localPort)
{
}

TcpClient::~TcpClient()
{
    if (socket_ != INVALID_SOCKET) {
        EventProcessor::GetInstance()->RemoveConnectionFd(socket_);
        close(socket_);
        socket_ = INVALID_SOCKET;
    }

    if (callbackThreads_) {
        callbackThreads_.reset();
    }
}

bool TcpClient::Init()
{
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET) {
        LOGE("Socket error: %s", strerror(errno));
        return false;
    }

    if (!localIp_.empty() || localPort_ != 0) {
        struct sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(localPort_);
        if (localIp_.empty()) {
            localIp_ = "0.0.0.0";
        }
        inet_aton(localIp_.c_str(), &localAddr.sin_addr);

        int optval = 1;
        if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            LOGE("setsockopt error: %s", strerror(errno));
            return false;
        }

        int ret = bind(socket_, (struct sockaddr *)&localAddr, (socklen_t)sizeof(localAddr));
        if (ret != 0) {
            LOGE("bind error: %s", strerror(errno));
            return false;
        }
    }

    return true;
}

bool TcpClient::Connect()
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(remotePort_);
    if (remoteIp_.empty()) {
        remoteIp_ = "127.0.0.1";
    }
    inet_aton(remoteIp_.c_str(), &serverAddr_.sin_addr);

    if (connect(socket_, (struct sockaddr *)&serverAddr_, sizeof(serverAddr_)) < 0) {
        LOGE("connect(%s:%d) failed: %s", remoteIp_.c_str(), remotePort_, strerror(errno));
        return false;
    }

    if (!listener_.expired()) {
        callbackThreads_ = std::make_unique<ThreadPool>(1, 1, "UdpClient-cb");
        EventProcessor::GetInstance()->AddConnectionFd(socket_, [&](int fd) { this->HandleReceive(fd); });
    }

    return true;
}

bool TcpClient::Send(const std::string &str)
{
    return TcpClient::Send(str.c_str(), str.length());
}

bool TcpClient::Send(const char *data, size_t len)
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    ssize_t bytes = ::send(socket_, data, len, 0);
    if (bytes != len) {
        LOGE("send failed: %s", strerror(errno));
        return false;
    }

    return true;
}

bool TcpClient::Send(std::shared_ptr<DataBuffer> data)
{
    return TcpClient::Send((char *)data->Data(), data->Size());
}

bool TcpClient::Close()
{
    if (socket_ != INVALID_SOCKET) {
        close(socket_);
        socket_ = INVALID_SOCKET;
    }
    return true;
}

void TcpClient::HandleReceive(int fd)
{
    LOGD("fd: %d", fd);
    static char buffer[RECV_BUFFER_MAX_SIZE] = {};
    memset(buffer, 0, RECV_BUFFER_MAX_SIZE);

    ssize_t nbytes = recv(fd, buffer, sizeof(buffer), 0);
    if (nbytes > 0) {
        if (!listener_.expired()) {
            auto dataBuffer = std::make_shared<DataBuffer>(nbytes);
            dataBuffer->Assign(buffer, nbytes);
            callbackThreads_->AddTask([=](void *) {
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnReceive(dataBuffer);
                }
            });
        }
    } else if (nbytes < 0) {
        std::string info = strerror(errno);
        LOGE("recv error: %s", info.c_str());
        EventProcessor::GetInstance()->RemoveConnectionFd(fd);
        close(fd);

        if (!listener_.expired()) {
            callbackThreads_->AddTask([=](void *) {
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnError(info);
                    Close();
                } else {
                    LOGE("not found listener!");
                }
            });
        }

    } else if (nbytes == 0) {
        LOGW("Disconnect fd[%d]", fd);
        EventProcessor::GetInstance()->RemoveConnectionFd(fd);
        close(fd);

        if (!listener_.expired()) {
            callbackThreads_->AddTask([=](void *) {
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnClose();
                    Close();
                } else {
                    LOGE("not found listener!");
                }
            });
        }
    }
}