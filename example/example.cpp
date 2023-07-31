#include <GulcarNet/GulcarNet.h>
#include <GulcarNet/Client.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

void ClientMain()
{
    GulcarNet::InitSockets();

    GulcarNet::Client client;
    client.Connect(GulcarNet::IPAddr("127.0.0.1", 6543));
    
    std::cout << (int)client.GetConnectionStatus() << "\n";

    while (true)
    {
        std::cout << "> ";
        std::string text;
        std::getline(std::cin, text);

        client.Send(text.c_str(), text.length());

        char buf[128] = {};
        int bytes = -1;

        while (bytes < 0)
        {
            bytes = client.Receive(buf, sizeof(buf));
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        std::cout << "received (" << bytes << " bytes): " << buf << "\n";
    }

    client.Disconnect();
    GulcarNet::ShutdownSockets();
}

void ServerMain()
{
    GulcarNet::InitSockets();

    GulcarNet::Socket sock;
    sock.Bind(6543);

    std::cout << "server started!\n";

    while (true)
    {
        GulcarNet::IPAddr clientAddr;

        char buf[128] = {};

        int bytes = sock.RecvFrom(buf, sizeof(buf), &clientAddr);

        if (bytes == GulcarNet::SockErr_ConnRefused)
        {
            std::cout << "disconnected!\n";
            break;
        }
        if (bytes < 0)
        {
            continue;
        }

        std::cout << "received (" << bytes << " bytes): " << buf << "\n";

        sock.SendTo(buf, bytes, clientAddr);
    }

    sock.Close();
    GulcarNet::ShutdownSockets();
}

int main()
{
    std::cout << "pozdravljen svet!\n";
    std::cout << "client/server? ";
    std::string input;
    std::cin >> input;

    if (input == "client" || input == "c")
    {
        ClientMain();
    }
    else if (input == "server" || input == "s")
    {
        ServerMain();
    }
    else
    {
        std::cout << "invalid input!\n";
    }
}