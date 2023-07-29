#include <GulcarNet/IPAddr.h>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

namespace GulcarNet
{
    IPAddr::IPAddr(const char* addr, uint16_t port)
    {
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(addr);
    }
}