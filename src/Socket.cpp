#include <GulcarNet/Socket.h>
#include <GulcarNet/IPAddr.h>
#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

namespace GulcarNet
{
    Socket::Socket()
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (m_socket == INVALID_SOCKET)
        {
            throw std::runtime_error("ERROR: socket failed!");
        }
    }

    void Socket::Bind(uint16_t port)
    {
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        int error = bind(m_socket, (sockaddr*)&addr, sizeof(addr));

        if (error != 0)
        {
            throw std::runtime_error("ERROR: bind failed!");
        }
    }

    size_t Socket::RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr)
    {
        int addrLen = sizeof(sockaddr_in);

        int bytesReceived = recvfrom(m_socket, (char*)outBuffer, outBufferSize, 0, (sockaddr*)outFromAddr->GetAddr(), &addrLen);

        if (bytesReceived == SOCKET_ERROR)
        {
            throw std::runtime_error("ERROR: recvfrom failed!");
        }

        return bytesReceived;
    }

    size_t Socket::SendTo(const void* data, size_t bytes, const IPAddr& addr)
    {
        int bytesSent = sendto(m_socket, (const char*)data, bytes, 0, (sockaddr*)addr.GetAddr(), sizeof(sockaddr_in));

        if (bytesSent == SOCKET_ERROR)
        {
            throw std::runtime_error("ERROR: sendto failed!");
        }

        return bytesSent;
    }

    void Socket::Close()
    {
        int error = closesocket(m_socket);

        if (error == SOCKET_ERROR)
        {
            throw std::runtime_error("ERROR: closesocket failed!");
        }
    }
}

