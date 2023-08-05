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

    client.SetDataReceiveCallback([](void* data, size_t bytes) {
        std::cout << "received (" << bytes << " bytes): " << (char*)data << "\n";
    });

    client.Connect(Net::IPAddr("127.0.0.1", 6543));
    client.Send("pozdrav", 7);

    std::mutex inputMutex;

    std::thread inputThr = std::thread([&client, &inputMutex]() {
        while (client.IsConnected())
        {
            std::string text;
            std::getline(std::cin, text);

            if (text.length() > 0)
            {
                std::lock_guard<std::mutex> guard(inputMutex);
                client.Send(text.c_str(), text.length());
            }
        }
    });

    while (client.IsConnected())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(17));

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
    server.SetDataReceiveCallback([&server](void* data, size_t bytes, Net::Connection&) {
        std::cout << "received (" << bytes << " bytes): " << (char*)data << "\n";
        server.SendToAll(data, bytes);
    });

    server.Start(6543);
    std::cout << "server started\n";

    while (true)
    {
        server.Process();
        std::this_thread::sleep_for(std::chrono::milliseconds(17));
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