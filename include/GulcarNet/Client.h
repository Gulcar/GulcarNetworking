#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

namespace Net
{
    class Client
    {
        using Clock = std::chrono::steady_clock;

    public:
        enum class Status
        {
            Disconnected,
            Connecting,
            Connected,
            FailedToConnect,
        };

        using StatusCallback = std::function<void(Status)>;
        using DataReceiveCallback = std::function<void(void* data, size_t bytes, uint16_t msgType)>;

    public:
        Client();
        ~Client();

        void Connect(const IPAddr& serverAddr);
        void Disconnect();

        void Send(Buf buf, uint16_t msgType, SendType reliable);

        void Process();

        void SetConnectionStatusCallback(StatusCallback callback);
        void SetDataReceiveCallback(DataReceiveCallback callback);

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline bool IsConnecting() const { return m_status == Status::Connecting; }
        inline Status GetConnectionStatus() const { return m_status; }

    private:
        void SetStatus(Status status);

    private:
        std::unique_ptr<class Socket> m_socket;
        std::unique_ptr<class Transport> m_transport;
        IPAddr m_serverAddr;

        Status m_status = Status::Disconnected;
        bool m_socketOpen = false;
        Clock::time_point m_connReqTime;

        StatusCallback m_statusCallback;
        DataReceiveCallback m_dataReceiveCallback;
    };
}
