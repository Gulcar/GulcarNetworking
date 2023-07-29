#pragma once

#include <GulcarNet/IPAddr.h>
#include <stdint.h>

namespace GulcarNet
{
    class Socket
    {
    public:

        Socket();

        void Bind(uint16_t port);

        size_t RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr);

        size_t Socket::SendTo(const void* data, size_t bytes, const IPAddr& addr);

        void Close();


    private:
        uint64_t m_socket = ~0;
    };
}
