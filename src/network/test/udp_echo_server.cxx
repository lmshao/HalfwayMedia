//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "../udp_server.h"
#include <iostream>
#include <thread>
#include <unistd.h>

class MyListener : public IServerListener {
public:
    void OnError(std::shared_ptr<Session> clientSession, const std::string &errorInfo) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnError: '" << errorInfo << "' from " << clientSession->ClientInfo() << std::endl;
        std::cout << "----------" << std::endl;
    }
    void OnClose(std::shared_ptr<Session> clientSession) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnClose: from " << clientSession->ClientInfo() << std::endl;
        std::cout << "----------" << std::endl;
    }
    void OnAccept(std::shared_ptr<Session> clientSession) override {}
    void OnReceive(std::shared_ptr<Session> clientSession, std::shared_ptr<DataBuffer> buffer) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnReceive " << buffer->Size() << " bytes from " << clientSession->ClientInfo() << std::endl;
        std::cout << buffer->Data() << std::endl;
        std::cout << "----------" << std::endl;
        if (clientSession->Send(buffer)) {
            std::cout << "send echo data ok." << std::endl;
        } else {
            std::cout << "send echo data failed." << std::endl;
        }
        std::cout << "----------" << std::endl;
    }
};

int main(int argc, char **argv)
{
    printf("Built at %s on %s.\n", __TIME__, __DATE__);

    uint16_t port = 7777;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        printf("Usage:\n%s <port> [default:7777]\n", argv[0]);
    }

    auto udp_server = UdpServer::Create("0.0.0.0", port);
    auto listener = std::make_shared<MyListener>();
    udp_server->SetListener(listener);
    udp_server->Init();
    udp_server->Start();
    printf("Listen on port 0.0.0.0:%d\n", port);

    while (true) {
        sleep(100);
    }
    printf("Exit...\n");
    return 0;
}