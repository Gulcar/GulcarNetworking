#include <iostream>
#include <string>
#include <GulcarNet/GulcarNet.h>
#include <GulcarNet/Socket.h>

void ClientMain()
{
    GulcarNet::Init();

    GulcarNet::Socket sock;

    while (true)
    {
        std::cout << "> ";
        std::string text;
        std::getline(std::cin, text);

        sock.SendTo(text.data(), text.length(), GulcarNet::IPAddr("127.0.0.1", 6543));

        char buf[128] = {};

        GulcarNet::IPAddr addr;
        size_t bytes = sock.RecvFrom(buf, sizeof(buf), &addr);

        std::cout << "received (" << bytes << " bytes): " << buf << "\n";
    }

    sock.Close();
    GulcarNet::Shutdown();
}

void ServerMain()
{
    GulcarNet::Init();

    GulcarNet::Socket sock;
    sock.Bind(6543);

    std::cout << "server started!\n";

    while (true)
    {
        GulcarNet::IPAddr clientAddr;

        char buf[128] = {};

        size_t bytes = sock.RecvFrom(buf, sizeof(buf), &clientAddr);

        std::cout << "received (" << bytes << " bytes): " << buf << "\n";

        sock.SendTo(buf, bytes, clientAddr);
    }

    sock.Close();
    GulcarNet::Shutdown();
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