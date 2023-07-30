#pragma once

#include <GulcarNet/IPAddr.h>
#include <stdint.h>

#ifdef _WIN32
    #define SOCKET uint64_t
    #define INVALID_SOCKET ~0
#else
    #define SOCKET int
    #define INVALID_SOCKET -1
#endif

namespace GulcarNet
{
    void InitSockets();
    void ShutdownSockets();

    class Socket
    {
    public:

        Socket();

        void Bind(uint16_t port);

        int RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr);

        int SendTo(const void* data, size_t bytes, const IPAddr& addr);

        void Close();

    private:
        SOCKET m_socket = INVALID_SOCKET;
    };
}
