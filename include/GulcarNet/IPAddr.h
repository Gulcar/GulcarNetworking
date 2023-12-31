#pragma once

#include <stdint.h>
#include <functional>
#include <ostream>
#include <string_view>

namespace Net
{
    /** struct that holds an IP address */
    struct IPAddr
    {
        /** default constructor: address 0.0.0.0, port 0 */
        IPAddr() { address = 0; port = 0; }
        /** example: Net::IPAddr("192.168.1.25", 6969) */
        IPAddr(const char* addr, uint16_t port);
        /** example: Net::IPAddr("192.168.1.25", 6969) */
        IPAddr(std::string_view addr, uint16_t port)
            : IPAddr(addr.data(), port) {}
        /** example: Net::IPAddr(192, 168, 1, 25, 6969) */
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
        std::string ToString() const;

        /** returns true if this is not 0.0.0.0 or 255.255.255.255 */
        bool IsValid() const;

        /** ip address in network byte order */
        uint32_t address;
        /** port in host byte order */
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
