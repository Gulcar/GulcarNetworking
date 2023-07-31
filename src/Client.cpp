#include <GulcarNet/Client.h>
#include <GulcarNet/Socket.h>

namespace GulcarNet
{
    void Client::Connect(const IPAddr& serverAddr)
    {
        m_socket = std::make_unique<Socket>();
        m_socket->SetBlocking(false);
        m_serverAddr = serverAddr;
        m_status = Status::Connecting;
        // TODO: tukaj nek handshake
        if (true)
            m_status = Status::Connected;
        else
            m_status = Status::FailedToConnect;
    }

    void Client::Disconnect()
    {
        // TODO: if m_socket is open ->
        m_socket->Close();

        m_status = Status::Disconnected;
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
            m_status = Status::Disconnected;

        if (bytes > 0 && addr != m_serverAddr)
            return Receive(outBuffer, outBufferSize);

        return bytes;
    }
}

