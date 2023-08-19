#include <GulcarNet/Client.h>
#include <GulcarNet/Server.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>

void ClientMain()
{
    Net::Client client;

    client.SetConnectionStatusCallback([](Net::Client::Status status) {
        switch (status)
        {
        case Net::Client::Status::Connected:
            std::cout << "Status: Connected\n"; break;
        case Net::Client::Status::Disconnected:
            std::cout << "Status: Disconnected\n"; break;
        case Net::Client::Status::FailedToConnect:
            std::cout << "Status: FailedToConnect\n"; break;
        case Net::Client::Status::Connecting:
            std::cout << "Status: Connecting\n"; break;
        }
    });

    client.SetDataReceiveCallback([](void* data, size_t bytes, uint16_t msgType) {
        std::cout << "received (" << bytes << " bytes): ";
        std::cout.write((char*)data, bytes);
        std::cout << "\n";
    });

    client.Connect(Net::IPAddr("127.0.0.1", 6543));

    std::mutex inputMutex;

    std::thread inputThr = std::thread([&client, &inputMutex]() {
        while (client.IsConnected() || client.IsConnecting())
        {
            std::string text;
            std::getline(std::cin, text);

            if (text.length() > 0)
            {
                std::lock_guard<std::mutex> guard(inputMutex);
                client.Send(Net::Buf(text), 6, Net::Reliable);
            }
        }
    });

    while (client.IsConnected() || client.IsConnecting())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(7));

        {
            std::lock_guard<std::mutex> guard(inputMutex);
            client.Process();
        }
    }

    inputThr.join();
    client.Disconnect();
}

void ServerMain()
{
    Net::Server server;

    server.SetClientConnectedCallback([](Net::Connection& conn) {
        std::cout << "new connection: " << conn.GetAddr() << "\n";
    });
    server.SetClientDisconnectedCallback([](Net::Connection& conn) {
        std::cout << "closed connection: " << conn.GetAddr() << "\n";
    });
    server.SetDataReceiveCallback([&server](void* data, size_t bytes, uint16_t msgType, Net::Connection& conn) {
        std::cout << "received (" << bytes << " bytes): ";
        std::cout.write((char*)data, bytes);
        std::cout << "\n";
        server.SendToAll(Net::Buf(data, bytes), msgType, Net::Reliable);
    });

    server.Start(6543);
    std::cout << "server started\n";

    while (true)
    {
        server.Process();
        std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }

    server.Stop();
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