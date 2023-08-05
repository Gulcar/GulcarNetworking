#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/ChannelType.h>
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
        using DataReceiveCallback = std::function<void(void* data, size_t bytes)>;

    public:
        Client();
        ~Client();

        void Connect(const IPAddr& serverAddr);
        void Disconnect();

        int Send(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType);

        template<typename T>
        int Send(const T& data, uint16_t channelId, uint16_t msgType)
        {
            return Send(&data, sizeof(T), channelId, msgType);
        }

        void Process();

        void SetChannel(uint16_t id, ChannelType type);

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
        IPAddr m_serverAddr;

        Status m_status = Status::Disconnected;
        bool m_socketOpen = false;

        std::vector<struct Channel> m_channels;

        StatusCallback m_statusCallback;
        DataReceiveCallback m_dataReceiveCallback;
    };
}
