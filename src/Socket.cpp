#include <GulcarNet/Socket.h>
#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Errors.h>
#include <stdexcept>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <unistd.h>
#endif


namespace GulcarNet
{
    Socket::Socket()
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (m_socket == INVALID_SOCKET)
        {
            GulcarNet::PrintError();
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
            GulcarNet::PrintError();
            throw std::runtime_error("ERROR: bind failed!");
        }
    }

    size_t Socket::RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr)
    {
#ifdef _WIN32
        int addrLen = sizeof(sockaddr_in);
#else
        socklen_t addrLen = sizeof(sockaddr_in);
#endif

        int bytesReceived = recvfrom(m_socket, (char*)outBuffer, outBufferSize, 0, (sockaddr*)outFromAddr->GetAddr(), &addrLen);

        if (bytesReceived == -1)
        {
            GulcarNet::PrintError();
            throw std::runtime_error("ERROR: recvfrom failed!");
        }

        return bytesReceived;
    }

    size_t Socket::SendTo(const void* data, size_t bytes, const IPAddr& addr)
    {
        int bytesSent = sendto(m_socket, (const char*)data, bytes, 0, (sockaddr*)addr.GetAddr(), sizeof(sockaddr_in));

        if (bytesSent == -1)
        {
            GulcarNet::PrintError();
            throw std::runtime_error("ERROR: sendto failed!");
        }

        return bytesSent;
    }

    void Socket::Close()
    {
#ifdef _WIN32
        int error = closesocket(m_socket);
#else
        int error = close(m_socket);
#endif

        if (error == -1)
        {
            GulcarNet::PrintError();
            throw std::runtime_error("ERROR: closesocket failed!");
        }
    }
}

