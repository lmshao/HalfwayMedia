//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "tcp_client.h"

bool TcpClient::Send(const std::string &str)
{
    return TcpClient::Send(str.c_str(), str.length());
}

bool TcpClient::Send(const char *data, size_t len)
{
    // Socket::send()
    return true;
}

bool TcpClient::Send(std::shared_ptr<DataBuffer> data)
{
    return TcpClient::Send((char *)data->Data(), data->Size());
}