#pragma once

#include <stdint.h>
#include <WinSock2.h>

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
