#include <GulcarNet/Socket.h>
#include <GulcarNet/IPAddr.h>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32

    #ifdef SOCKET
        #undef SOCKET
    #endif
    #ifdef INVALID_SOCKET
        #undef INVALID_SOCKET
    #endif

    #define WIN32_LEAN_AND_MEAN
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")

#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <errno.h>
#endif


namespace GulcarNet
{
    void PrintError();
    void PrintErrorWS(int error);

    void InitSockets()
    {
#ifdef _WIN32
        WSADATA wsaData;
        int error = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (error != 0)
        {
            PrintErrorWS(error);
            throw std::runtime_error("ERROR: WSAStartup failed");
        }
#endif
    }

    void ShutdownSockets()
    {
#ifdef _WIN32
        int error = WSACleanup();

        if (error != 0)
        {
            PrintError();
            throw std::runtime_error("ERROR: WSACleanup failed");
        }
#endif
    }

    Socket::Socket()
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (m_socket == INVALID_SOCKET)
        {
            PrintError();
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
            PrintError();
            throw std::runtime_error("ERROR: bind failed!");
        }
    }

    int Socket::RecvFrom(void* outBuffer, size_t outBufferSize, IPAddr* outFromAddr)
    {
#ifdef _WIN32
        int addrLen = sizeof(sockaddr_in);
#else
        socklen_t addrLen = sizeof(sockaddr_in);
#endif
        sockaddr_in saddr = {};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(outFromAddr->port);
        saddr.sin_addr.s_addr = outFromAddr->address;

        int bytesReceived = recvfrom(m_socket, (char*)outBuffer, outBufferSize, 0, (sockaddr*)&saddr, &addrLen);

        outFromAddr->address = saddr.sin_addr.s_addr;
        outFromAddr->port = ntohs(saddr.sin_port);

        if (bytesReceived == -1)
        {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAECONNRESET)
            {
                PrintErrorWS(error);
                throw std::runtime_error("ERROR: recvfrom failed!");
            }
#else
            if (errno != ECONNREFUSED)
            {
                PrintError();
                throw std::runtime_error("ERROR: recvfrom failed!");
            }
#endif
        }

        return bytesReceived;
    }

    int Socket::SendTo(const void* data, size_t bytes, const IPAddr& addr)
    {
        sockaddr_in saddr = {};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(addr.port);
        saddr.sin_addr.s_addr = addr.address;

        int bytesSent = sendto(m_socket, (const char*)data, bytes, 0, (const sockaddr*)&saddr, sizeof(sockaddr_in));

        if (bytesSent == -1)
        {
            PrintError();
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
            PrintError();
            throw std::runtime_error("ERROR: closesocket failed!");
        }
    }

    void PrintError()
    {
#ifdef _WIN32
        int error = WSAGetLastError();
        PrintErrorWS(error);
#else
        perror("GulcarNet ERROR");
#endif
    }

    void PrintErrorWS(int error)
    {
#ifdef _WIN32
        char msg[256] = {};

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msg,
            sizeof(msg),
            nullptr);

        if (msg[0] == '\0')
        {
            std::cout << "GulcarNet ERROR: " << error << "\n";
        }
        else
        {
            std::cout << "GulcarNet ERROR: " << msg;
            std::cout << "error code: " << error << "\n";
        }
#endif
    }
}

