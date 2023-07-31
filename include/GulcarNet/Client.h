#pragma once

#include <GulcarNet/IPAddr.h>
#include <memory>

namespace GulcarNet
{
    class Client
    {
    public:
        enum class Status
        {
            Disconnected,
            Connecting,
            Connected,
            FailedToConnect,
        };

    public:
        void Connect(const IPAddr& serverAddr);
        void Disconnect();

        int Send(const void* data, size_t bytes);

        template<typename T>
        int Send(const T& data)
        {
            return Send(&data, sizeof(T));
        }

        int Receive(void* outBuffer, size_t outBufferSize);

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline Status GetConnectionStatus() const { return m_status; }

    private:
        std::unique_ptr<class Socket> m_socket;
        IPAddr m_serverAddr;
        Status m_status = Status::Disconnected;
    };
}
