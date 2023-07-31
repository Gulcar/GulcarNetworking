#pragma once

#include <GulcarNet/IPAddr.h>
#include <memory>
#include <functional>

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

        using StatusCallback = std::function<void(Status)>;

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

        void SetConnectionStatusCallback(StatusCallback callback);

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline Status GetConnectionStatus() const { return m_status; }

    private:
        void SetStatus(Status status);

    private:
        std::unique_ptr<class Socket> m_socket;
        IPAddr m_serverAddr;
        Status m_status = Status::Disconnected;
        StatusCallback m_statusCallback;
    };
}
