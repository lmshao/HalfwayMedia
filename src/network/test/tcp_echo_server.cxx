//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "../tcp_server.h"

#include <stdio.h>
#include <thread>
#include <unistd.h>

class MyListener : public IServerListener {
public:
    void OnError(const ClientInfo &client) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnError: from " << client.ToString() << std::endl;
        std::cout << "----------" << std::endl;
    }
    void OnClose(const ClientInfo &client) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnClose: from " << client.ToString() << std::endl;
        std::cout << "----------" << std::endl;
    }
    void OnAccept(const ClientInfo &client) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnAccept: from " << client.ToString() << std::endl;
        std::cout << "----------" << std::endl;
    }
    void OnReceive(const ClientInfo &client, std::shared_ptr<DataBuffer> buffer) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnReceive " << buffer->Size() << "bytes from " << client.ToString() << std::endl;
        std::cout << buffer->Data() << std::endl;
        std::cout << "----------" << std::endl;
    }
};

int main(int argc, char **argv)
{
    printf("Built at %s on %s.\n", __TIME__, __DATE__);

    auto tcp_server = TcpServer::Create("0.0.0.0", 7778);
    auto listener = std::make_shared<MyListener>();
    tcp_server->SetListener(listener);
    tcp_server->Init();
    tcp_server->Start();

    printf("Waiting...\n");
    sleep(200);
    printf("Exit...\n");
    return 0;
}