#include <GulcarNet/GulcarNet.h>
#include <iostream>
#include <string>

void ClientMain()
{
    GulcarNet::InitSockets();

    GulcarNet::Socket sock;

    while (true)
    {
        std::cout << "> ";
        std::string text;
        std::getline(std::cin, text);

        sock.SendTo(text.data(), text.length(), GulcarNet::IPAddr("127.0.0.1", 6543));

        char buf[128] = {};

        GulcarNet::IPAddr addr;
        int bytes = sock.RecvFrom(buf, sizeof(buf), &addr);

        if (bytes == GulcarNet::SockErr_ConnRefused)
        {
            std::cout << "disconnected!\n";
            break;
        }

        std::cout << "received (" << bytes << " bytes): " << buf << "\n";
    }

    sock.Close();
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