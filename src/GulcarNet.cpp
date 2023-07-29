#include <GulcarNet/GulcarNet.h>
#include <stdexcept>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

namespace GulcarNet
{
    void Init()
    {
        WSADATA wsaData;
        int error = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (error != 0)
        {
            throw std::runtime_error("ERROR: WSAStartup failed");
        }
    }

    void Shutdown()
    {
        int error = WSACleanup();

        if (error != 0)
        {
            throw std::runtime_error("ERROR: WSACleanup failed");
        }
    }
}
