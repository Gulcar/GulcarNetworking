#include <GulcarNet/Client.h>
#include "Socket.h"
#include "Transport.h"
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

        m_transport = std::make_unique<Transport>(m_socket.get(), m_serverAddr);

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

    void Client::Send(Buf buf, uint16_t msgType, SendType reliable)
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");
        m_transport->Send(buf.data, buf.bytes, msgType, reliable);
    }

    void Client::Process()
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");

        m_transport->SendExtraAcks();

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

#ifdef GULCAR_NET_SIM_PACKET_LOSS
            if (rand() % 100 < 3)
                continue;
#endif
            buf[bytes] = '\0';

            Transport::ReceiveData receiveData = m_transport->Receive(buf, bytes);

            if (receiveData.callback && m_dataReceiveCallback)
                m_dataReceiveCallback(receiveData.data, receiveData.bytes, receiveData.msgType);
        }

        m_transport->RetrySending();
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

