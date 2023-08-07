#include <GulcarNet/IPAddr.h>
#include <sstream>

#ifdef _WIN32
    #include <WinSock2.h>
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace Net
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

    std::ostream& operator<<(std::ostream& os, IPAddr addr)
    {
        os << (addr.address & 0xff) << '.';
        os << ((addr.address >> 8) & 0xff) << '.';
        os << ((addr.address >> 16) & 0xff) << '.';
        os << ((addr.address >> 24) & 0xff) << ':';
        os << addr.port;
        return os;
    }

    std::string IPAddr::ToString() const
    {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
}
