#include <GulcarNet/Client.h>
#include "Socket.h"
#include <cassert>

#ifndef GULCAR_NET_RECV_BUF_SIZE
#define GULCAR_NET_RECV_BUF_SIZE 512
#endif

namespace Net
{
    Client::Client()
    {
        Net::InitSockets();
    }

    Client::~Client()
    {
        Net::ShutdownSockets();
    }

    void Client::Connect(const IPAddr& serverAddr)
    {
        if (m_socket)
            m_socket->Close();

        m_socket = std::make_unique<Socket>();
        m_socket->SetBlocking(false);
        m_serverAddr = serverAddr;
        m_socketOpen = true;

        SetStatus(Status::Connecting);
        // TODO: tukaj nek handshake
        if (true)
            SetStatus(Status::Connected);
        else
            SetStatus(Status::FailedToConnect);
    }

    void Client::Disconnect()
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");

        m_socket->Close();

        SetStatus(Status::Disconnected);
        // TODO: poslji serverju disconnect
    }

    int Client::Send(const void* data, size_t bytes)
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");
        return m_socket->SendTo(data, bytes, m_serverAddr);
    }

    void Client::Process()
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");

        char buf[GULCAR_NET_RECV_BUF_SIZE];

        for (int i = 0; i < 256; i++)
        {
            IPAddr addr;
            int bytes = m_socket->RecvFrom(buf, sizeof(buf), &addr);

            if (bytes == SockErr_WouldBlock)
                break;
            else if (bytes == SockErr_ConnRefused)
                SetStatus(Status::Disconnected);

            if (bytes <= 0)
                continue;
            if (addr != m_serverAddr)
                continue;

            buf[bytes] = '\0';

            if (m_dataReceiveCallback)
                m_dataReceiveCallback(buf, bytes);
        }
    }

    void Client::SetConnectionStatusCallback(StatusCallback callback)
    {
        m_statusCallback = callback;
    }

    void Client::SetDataReceiveCallback(DataReceiveCallback callback)
    {
        m_dataReceiveCallback = callback;
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

