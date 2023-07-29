#include <GulcarNet/GulcarNet.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace GulcarNet
{
    void Init()
    {
#ifdef _WIN32
        WSADATA wsaData;
        int error = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (error != 0)
        {
            throw std::runtime_error("ERROR: WSAStartup failed");
        }
#endif
    }

    void Shutdown()
    {
#ifdef _WIN32
        int error = WSACleanup();

        if (error != 0)
        {
            throw std::runtime_error("ERROR: WSACleanup failed");
        }
#endif
    }
}
