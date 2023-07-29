#pragma once

#include <GulcarNet/IPAddr.h>

#ifdef _WIN32
    #ifdef INVALID_SOCKET
        #undef INVALID_SOCKET
        #define INVALID_SOCKET ~0
    #endif
#else
    #define SOCKET int
    #define INVALID_SOCKET -1
#endif

namespace GulcarNet
{
    class Socket
    {
    public:

        Socket();

        void Bind(uint16_t port);

        size_t RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr);

        size_t SendTo(const void* data, size_t bytes, const IPAddr& addr);

        void Close();


    private:
        SOCKET m_socket = INVALID_SOCKET;
    };
}
