#include <GulcarNet/IPAddr.h>

#ifdef _WIN32
    #include <WinSock2.h>
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace GulcarNet
{
    IPAddr::IPAddr(const char* addr, uint16_t port)
    {
        this->address = inet_addr(addr);
        this->port = port;
    }

    IPAddr::IPAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
    {
        this->address = a + (b << 8) + (c << 16) + (d << 24);
        this->port = port;
    }
}
