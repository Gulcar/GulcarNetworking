#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

namespace Net
{
    /** network interface for clients */
    class Client
    {
        using Clock = std::chrono::steady_clock;

    public:
        /** The connection status. Use GetConnectionStatus() to retrieve or bind a function to SetConnectionStatusCallback(). */
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

        /**
        * Tries connecting to the server.
        * 
        * You will need to call Process() for this to work.
        * Use SetDataReceiveCallback() to be notified of the connection status.
        */
        void Connect(const IPAddr& serverAddr);

        /**
        * Disconnects from the server.
        * 
        * You can still call Process() and you can call Connect() again.
        */
        void Disconnect();

        /**
        * Sends a message to the server.
        * @param buf message to send
        * @param msgType type of the message usually to identify the struct of data you are sending
        * @param reliable enum of how to manage reliability
        */
        void Send(Buf buf, uint16_t msgType, SendType reliable);

        /**
        * Call this every frame.
        * 
        * It will receive data on the socket and manage acks.
        * Your callbacks will be called in this function.
        */
        void Process();

        /** Sets the callback which will receive status updates. Will be called from Process(). */
        void SetConnectionStatusCallback(StatusCallback callback);
        /** Sets the callback which will receive data that was received from the server. Will be called from Process(). */
        void SetDataReceiveCallback(DataReceiveCallback callback);

        /** Returns true if GetConnectionStatus() is Status::Connected. */
        inline bool IsConnected() const { return m_status == Status::Connected; }
        /** Returns true if GetConnectionStatus() is Status::Connecting. */
        inline bool IsConnecting() const { return m_status == Status::Connecting; }
        /** Returns the current Status of the connection. Consider using SetConnectionStatusCallback(). */
        inline Status GetConnectionStatus() const { return m_status; }

        inline const Statistics& GetStats() const { return m_stats; }

    private:
        void SetStatus(Status status);

    private:
        std::unique_ptr<class Socket> m_socket;
        std::unique_ptr<class Transport> m_transport;
        IPAddr m_serverAddr;

        Status m_status = Status::Disconnected;
        Clock::time_point m_connReqTime;

        StatusCallback m_statusCallback;
        DataReceiveCallback m_dataReceiveCallback;

        Statistics m_stats;
    };
}
