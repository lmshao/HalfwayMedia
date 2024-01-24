//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "../tcp_client.h"

#include <assert.h>
#include <stdio.h>
#include <thread>

bool gExit = false;
class MyListener : public IClientListener {
public:
    void OnReceive(std::shared_ptr<DataBuffer> buffer) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnReceive " << buffer->Size() << " bytes:" << std::endl;
        std::cout << buffer->Data() << std::endl;
        std::cout << "----------" << std::endl;
    }

    void OnClose() override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "Server close connection" << std::endl;
        std::cout << "----------" << std::endl;
        gExit = true;
    }

    void OnError(const std::string &errorInfo) override
    {
        std::cout << "----------" << std::endl;
        std::cout << "pid: " << std::this_thread::get_id() << std::endl;
        std::cout << "OnError: '" << errorInfo << "'" << std::endl;
        std::cout << "----------" << std::endl;
        gExit = true;
    }
};

int main(int argc, char **argv)
{
    printf("Built at %s on %s.\n", __TIME__, __DATE__);

    std::string remoteIp = "127.0.0.1";
    uint16_t remotePort;

    if (argc > 3 || argc == 1) {
        printf("param err:\nUsage:\n%s <ip> <port> | %s <port>\n", argv[0], argv[0]);
        return -1;
    } else if (argc == 3) {
        remoteIp = argv[1];
        remotePort = atoi(argv[2]);
    } else if (argc == 2) {
        remotePort = atoi(argv[1]);
    }

    auto tcpClient = TcpClient::Create(remoteIp, remotePort, "", 0);
    auto listener = std::make_shared<MyListener>();
    tcpClient->SetListener(listener);
    bool res = false;
    res = tcpClient->Init();
    assert(res);
    res = tcpClient->Connect();
    assert(res);
    printf("----\n");
    char sendbuf[1024];
    printf("Input:\n----------\n");

    while (!gExit && fgets(sendbuf, sizeof(sendbuf), stdin)) {
        if (tcpClient->Send(sendbuf, strlen(sendbuf))) {
            std::cout << "Send scuccess, length: " << strlen(sendbuf) << std::endl;
        } else {
            std::cout << "Send error" << std::endl;
        }

        memset(sendbuf, 0, sizeof(sendbuf));
        printf("Input:\n----------\n");
    }

    tcpClient->Close();
    return 0;
}