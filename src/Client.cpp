#include <GulcarNet/Client.h>
#include <GulcarNet/Socket.h>

namespace GulcarNet
{
    void Client::Connect(const IPAddr& serverAddr)
    {
        m_socket = std::make_unique<Socket>();
        m_socket->SetBlocking(false);
        m_serverAddr = serverAddr;
        SetStatus(Status::Connecting);
        // TODO: tukaj nek handshake
        if (true)
            SetStatus(Status::Connected);
        else
            SetStatus(Status::FailedToConnect);
    }

    void Client::Disconnect()
    {
        // TODO: if m_socket is open ->
        m_socket->Close();

        SetStatus(Status::Disconnected);
        // TODO: poslji serverju disconnect
    }

    int Client::Send(const void* data, size_t bytes)
    {
        return m_socket->SendTo(data, bytes, m_serverAddr);
    }

    int Client::Receive(void* outBuffer, size_t outBufferSize)
    {
        IPAddr addr;
        int bytes = m_socket->RecvFrom(outBuffer, outBufferSize, &addr);

        if (bytes == SockErr_ConnRefused)
            SetStatus(Status::Disconnected);

        if (bytes > 0 && addr != m_serverAddr)
            return Receive(outBuffer, outBufferSize);

        return bytes;
    }

    void Client::SetConnectionStatusCallback(StatusCallback callback)
    {
        m_statusCallback = callback;
    }

    void Client::SetStatus(Status status)
    {
        if (m_status == status)
            return;

        m_status = status;

        if (m_statusCallback)
            m_statusCallback(status);
    }
}

