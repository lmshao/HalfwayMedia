//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "tcp_server.h"
#include "../common/thread_pool.h"
#include "../common/utils.h"
#include "../network/network_common.h"
#include "event_processor.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>

#define RECV_BUFFER_MAX_SIZE 4096

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

    EventProcessor::GetInstance()->AddListeningFd(socket_, shared_from_this());

    return true;
}

bool TcpServer::Stop()
{
    LOGD("enter");
    if (socket_ != INVALID_SOCKET) {
        callbackThreads_.reset();
        EventProcessor::GetInstance()->RemoveListeningFd(socket_);
        socket_ = INVALID_SOCKET;
    }
    LOGD("leave");
    return true;
}

void TcpServer::OnAccept(const ClientInfo &client)
{
    LOGD("onAccept");
    if (!listener_.expired()) {
        callbackThreads_->AddTask([=](void *) {
            LOGD("invoke OnAccept callback");
            auto listener = listener_.lock();
            if (listener) {
                listener->OnAccept(client);
            } else {
                LOGE("not found listener!");
            }
        });
    } else {
        LOGE("listener is null");
    }
}

void TcpServer::OnReceive(const ClientInfo &client)
{
    LOGD("OnReceive from %s:%d", client.host.c_str(), client.port);
    static char buffer[RECV_BUFFER_MAX_SIZE] = {};
    memset(buffer, 0, RECV_BUFFER_MAX_SIZE);

    ssize_t nbytes = recv(client.fd, buffer, sizeof(buffer), 0);
    if (nbytes < 0) {
        LOGE("recv error: %s", strerror(errno));
    } else if (nbytes == 0) {
        LOGW("Disconnect fd[%d]", client.fd);
        close(client.fd);

        EventProcessor::GetInstance()->RemoveConnectionFd(client.fd);

        if (!listener_.expired()) {
            callbackThreads_->AddTask([=](void *) {
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnClose(client);
                } else {
                    LOGE("not found listener!");
                }
            });
        }
    } else {
        LOGD("...");
        if (!listener_.expired()) {
            auto dataBuffer = std::make_shared<DataBuffer>(nbytes);
            dataBuffer->Assign(buffer, nbytes);
            callbackThreads_->AddTask([=](void *) {
                auto listener = listener_.lock();
                if (listener) {
                    listener->OnReceive(client, dataBuffer);
                }
            });
        }
    }
    LOGD("...");
}