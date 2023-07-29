#include <GulcarNet/Errors.h>
#include <iostream>
#include <stdio.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")
#endif

namespace GulcarNet
{
    void PrintError()
    {
#ifdef _WIN32
        int error = WSAGetLastError();
        PrintErrorWS(error);
#else
        perror("GulcarNet ERROR");
#endif
    }

    void PrintErrorWS(int error)
    {
#ifdef _WIN32
        char msg[256] = {};

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msg,
            sizeof(msg),
            nullptr);

        if (msg[0] == '\0')
            std::cout << "GulcarNet ERROR: " << error << "\n";
        else
            std::cout << "GulcarNet ERROR: " << msg << "\n";
#endif
    }
}
