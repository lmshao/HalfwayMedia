//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "udp_server.h"
#include "../common/log.h"
#include "../common/thread_pool.h"
#include "event_processor.h"
#include <arpa/inet.h>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const int RECV_BUFFER_MAX_SIZE = 4096;
const uint16_t UDP_SERVER_DEFAULT_PORT_START = 10000;

static uint16_t idlePort_ = UDP_SERVER_DEFAULT_PORT_START;

UdpServer::~UdpServer()
{
    LOGD("destructor");
    Stop();
}

bool UdpServer::Init()
{
    socket_ = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket error: %s", strerror(errno));
        return false;
    }
    LOGD("fd:%d", socket_);

    int optval = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOGE("setsockopt error: %s", strerror(errno));
        return false;
    }

    // int bufferSize = 819200;
    // if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) < 0) {
    //     LOGE("setsockopt error: %s", strerror(errno));
    //     return false;
    // }

    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(localPort_);
    inet_aton(localIp_.c_str(), &serverAddr_.sin_addr);

    if (bind(socket_, (struct sockaddr *)&serverAddr_, sizeof(serverAddr_)) < 0) {
        LOGE("bind error: %s", strerror(errno));
        return false;
    }

    return true;
}

bool UdpServer::Start()
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    if (!listener_.expired()) {
        callbackThreads_ = std::make_unique<ThreadPool>(1, 2, "UdpServer-cb");
    }

    EventProcessor::GetInstance()->AddServiceFd(socket_, [&](int fd) { this->HandleReceive(fd); });

    return true;
}

bool UdpServer::Stop()
{
    if (socket_ != INVALID_SOCKET) {
        EventProcessor::GetInstance()->RemoveServiceFd(socket_);
        close(socket_);
        socket_ = INVALID_SOCKET;
        callbackThreads_.reset();
    }

    return true;
}

bool UdpServer::Send(int fd, std::string host, uint16_t port, std::shared_ptr<DataBuffer> buffer)
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    inet_aton(host.c_str(), &remoteAddr.sin_addr);

    ssize_t nbytes =
        sendto(socket_, buffer->Data(), buffer->Size(), 0, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr));
    if (nbytes != buffer->Size()) {
        LOGE("send error: %s %zd/%zu", strerror(errno), nbytes, buffer->Size());
        return false;
    }

    return true;
}

void UdpServer::HandleReceive(int fd)
{
    static char buffer[RECV_BUFFER_MAX_SIZE] = {};
    memset(buffer, 0, RECV_BUFFER_MAX_SIZE);
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    while (true) {
        ssize_t nbytes = recvfrom(socket_, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrLen);
        std::string host = inet_ntoa(clientAddr.sin_addr);
        uint16_t port = ntohs(clientAddr.sin_port);
        // LOGD("recvfrom %s:%d", host.c_str(), port);

        if (nbytes > 0) {
            if (!listener_.expired()) {
                auto dataBuffer = std::make_shared<DataBuffer>(nbytes);
                dataBuffer->Assign(buffer, nbytes);
                callbackThreads_->AddTask(
                    [=](void *) {
                        auto listener = listener_.lock();
                        if (listener) {
                            listener->OnReceive(std::make_shared<SessionImpl>(fd, host, port, shared_from_this()),
                                                dataBuffer);
                        }
                    },
                    host + std::to_string(port));
            }
            continue;
        }

        if (nbytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }

        std::string info = strerror(errno);
        LOGE("recvfrom() failed: %s", info.c_str());
        if (!listener_.expired()) {
            callbackThreads_->AddTask(
                [=](void *) {
                    auto listener = listener_.lock();
                    if (listener) {
                        listener->OnError(std::make_shared<SessionImpl>(fd, host, port, shared_from_this()), info);
                        sessions_.erase(fd);
                    } else {
                        LOGE("not found listener!");
                    }
                },
                host + std::to_string(port));
        }
        break;
    }
}

uint16_t UdpServer::GetIdlePort()
{
    int sock;
    struct sockaddr_in addr;

    uint16_t i;
    for (i = idlePort_; i < idlePort_ + 100; i++) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("socket creation failed");
            return -1;
        }

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(i);

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(sock);
            continue;
        }

        close(sock);
        break;
    }

    if (i == idlePort_ + 100) {
        LOGE("Can't find idle port");
        return 0;
    }

    idlePort_ = i + 1;

    return i;
}

uint16_t UdpServer::GetIdlePortPair()
{
    uint16_t firstPort;
    uint16_t secondPort;
    firstPort = GetIdlePort();

    while (true) {
        secondPort = GetIdlePort();
        if (firstPort + 1 == secondPort || secondPort == 0) {
            return firstPort;
        } else {
            firstPort = secondPort;
        }
    }
}
