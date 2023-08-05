#pragma once

#include <stdint.h>
#include <functional>
#include <ostream>

namespace Net
{
    struct IPAddr
    {
        IPAddr() { address = 0; port = 0; }
        IPAddr(const char* addr, uint16_t port);
        IPAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);

        friend inline bool operator==(const IPAddr& lhs, const IPAddr& rhs)
        {
            return (lhs.address == rhs.address) &&
                (lhs.port == rhs.port);
        }

        friend inline bool operator!=(const IPAddr& lhs, const IPAddr& rhs)
        {
            return !(lhs == rhs);
        }

        friend std::ostream& operator<<(std::ostream& os, IPAddr addr);
        std::string ToString();

        uint32_t address;
        uint16_t port;
    };
}

template <>
struct std::hash<Net::IPAddr>
{
    uint64_t operator()(const Net::IPAddr& addr) const
    {
        return ((uint64_t)addr.address << 16) | addr.port;
    }
};
