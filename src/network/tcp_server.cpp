//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "tcp_server.h"
#include "../common/log.h"
#include "../common/thread_pool.h"
#include "event_processor.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const int RECV_BUFFER_MAX_SIZE = 4096;
const int TCP_BACKLOG = 10;

TcpServer::~TcpServer()
{
    Stop();
}

bool TcpServer::Init()
{
    socket_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket error: %s", strerror(errno));
        return false;
    }
    LOGD("... fd:%d", socket_);

    int optval = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOGE("setsockopt error: %s", strerror(errno));
        return false;
    }

    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(localPort_);
    inet_aton(localIp_.c_str(), &serverAddr_.sin_addr);

    if (bind(socket_, (struct sockaddr *)&serverAddr_, sizeof(serverAddr_)) < 0) {
        LOGE("bind error: %s", strerror(errno));
        return false;
    }

    if (listen(socket_, TCP_BACKLOG) < 0) {
        LOGE("listen error: %s", strerror(errno));
        return false;
    }

    return true;
}

bool TcpServer::Start()
{
    if (socket_ == INVALID_SOCKET) {
        LOGE("socket not initialized");
        return false;
    }

    if (!listener_.expired()) {
        callbackThreads_ = std::make_unique<ThreadPool>(1, 4, "TcpServer-cb");
    }

    EventProcessor::GetInstance()->AddServiceFd(socket_, [&](int fd) { this->HandleAccept(fd); });

    return true;
}

bool TcpServer::Stop()
{
    if (socket_ != INVALID_SOCKET) {
        callbackThreads_.reset();
        EventProcessor::GetInstance()->RemoveServiceFd(socket_);
        close(socket_);
        socket_ = INVALID_SOCKET;
    }

    return true;
}

bool TcpServer::Send(int fd, std::string host, uint16_t port, std::shared_ptr<DataBuffer> buffer)
{
    if (sessions_.find(fd) == sessions_.end()) {
        LOGE("invalid session fd");
        return false;
    }

    ssize_t bytes = send(fd, buffer->Data(), buffer->Size(), 0);
    if (bytes != buffer->Size()) {
        printf("Send scuccess, length:%zd\n\n", bytes);
        LOGE("send failed with error: %s, %zd/%zu", strerror(errno), bytes, buffer->Size());
        return false;
    }

    return true;
}

void TcpServer::HandleAccept(int fd)
{
    LOGD("enter");
    struct sockaddr_in clientAddr = {};
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int clientSocket = accept(fd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSocket < 0) {
        LOGE("accept error: %s", strerror(errno));
        return;
    }

    EventProcessor::GetInstance()->AddConnectionFd(clientSocket, [&](int fd) { this->HandleReceive(fd); });

    std::string host = inet_ntoa(clientAddr.sin_addr);
    uint16_t port = ntohs(clientAddr.sin_port);

    LOGD("New client connections client[%d] %s:%d\n", clientSocket, inet_ntoa(clientAddr.sin_addr),
         ntohs(clientAddr.sin_port));

    auto session = std::make_shared<SessionImpl>(clientSocket, host, port, shared_from_this());
    sessions_.emplace(clientSocket, session);

    if (!listener_.expired()) {
        callbackThreads_->AddTask(
            [=](void *) {
                LOGD("invoke OnAccept callback");
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnAccept(sessions_[clientSocket]);
                } else {
                    LOGE("not found listener!");
                }
            },
            std::to_string(fd));
    } else {
        LOGE("listener is null");
    }
}

void TcpServer::HandleReceive(int fd)
{
    LOGD("fd: %d", fd);
    static char buffer[RECV_BUFFER_MAX_SIZE] = {};
    memset(buffer, 0, RECV_BUFFER_MAX_SIZE);

    while (true) {
        ssize_t nbytes = recv(fd, buffer, sizeof(buffer), 0);
        if (nbytes > 0) {
            if (!listener_.expired()) {
                auto dataBuffer = std::make_shared<DataBuffer>(nbytes);
                dataBuffer->Assign(buffer, nbytes);
                callbackThreads_->AddTask(
                    [=](void *) {
                        auto listener = listener_.lock();
                        if (listener) {
                            listener->OnReceive(sessions_[fd], dataBuffer);
                        }
                    },
                    std::to_string(fd));
            }
            continue;
        }

        if (nbytes == 0) {
            LOGW("Disconnect fd[%d]", fd);
            EventProcessor::GetInstance()->RemoveConnectionFd(fd);
            close(fd);

            if (!listener_.expired()) {
                callbackThreads_->AddTask(
                    [=](void *) {
                        auto listener = listener_.lock();
                        if (listener) {
                            listener->OnClose(sessions_[fd]);
                            sessions_.erase(fd);
                        } else {
                            LOGE("not found listener!");
                        }
                    },
                    std::to_string(fd));
            }

        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            std::string info = strerror(errno);
            LOGE("recv error: %s", info.c_str());
            EventProcessor::GetInstance()->RemoveConnectionFd(fd);
            close(fd);

            if (!listener_.expired()) {
                callbackThreads_->AddTask(
                    [=](void *) {
                        auto listener = listener_.lock();
                        if (listener) {
                            listener->OnError(sessions_[fd], info);
                            sessions_.erase(fd);
                        } else {
                            LOGE("not found listener!");
                        }
                    },
                    std::to_string(fd));
            }
        }

        break;
    }
}