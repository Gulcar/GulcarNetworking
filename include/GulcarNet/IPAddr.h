#pragma once

#include <stdint.h>

#ifdef _WIN32
    #include <WinSock2.h>
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace GulcarNet
{
    class IPAddr
    {
    public:
        IPAddr() { }
        IPAddr(const char* addr, uint16_t port);

        inline sockaddr_in* GetAddr() { return &m_addr; }
        inline const sockaddr_in* GetAddr() const { return &m_addr; }

    private:
        sockaddr_in m_addr = {};
    };
}
