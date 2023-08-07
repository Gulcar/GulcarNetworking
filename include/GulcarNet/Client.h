#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <memory>
#include <functional>
#include <vector>

namespace Net
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
        using DataReceiveCallback = std::function<void(void* data, size_t bytes, uint16_t msgType)>;

    public:
        Client();
        ~Client();

        void Connect(const IPAddr& serverAddr);
        void Disconnect();

        int Send(Buf buf, uint16_t msgType, SendType reliable);

        void Process();

        void SetConnectionStatusCallback(StatusCallback callback);
        void SetDataReceiveCallback(DataReceiveCallback callback);

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline Status GetConnectionStatus() const { return m_status; }

    private:
        void SetStatus(Status status);

        int SendUnreliable(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType);
        int SendUnreliableDiscardOld(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType);
        int SendReliable(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType);

    private:
        std::unique_ptr<class Socket> m_socket;
        std::unique_ptr<class Transport> m_transport;
        IPAddr m_serverAddr;

        Status m_status = Status::Disconnected;
        bool m_socketOpen = false;

        StatusCallback m_statusCallback;
        DataReceiveCallback m_dataReceiveCallback;
    };
}
