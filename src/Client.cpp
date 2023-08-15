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

        m_transport = std::make_unique<Transport>(m_socket.get(), m_serverAddr);

        SetStatus(Status::Connecting);
        m_transport->SendConnectRequest();
        m_connReqTime = Clock::now();
    }

    void Client::Disconnect()
    {
        if (m_socket)
        {
            m_socket->Close();
            m_socket = nullptr;
        }

        SetStatus(Status::Disconnected);
    }

    void Client::Send(Buf buf, uint16_t msgType, SendType reliable)
    {
        assert(m_socket && "GulcarNet: Client Connect not called!");
        m_transport->Send(buf.data, buf.bytes, msgType, reliable);
    }

    void Client::Process()
    {
        if (!m_socket || m_status == Status::FailedToConnect || m_status == Status::Disconnected)
            return;

        m_transport->SendExtraAcks();

        char buf[GULCAR_NET_RECV_BUF_SIZE];

        for (int i = 0; i < 256; i++)
        {
            IPAddr addr;
            int bytes = m_socket->RecvFrom(buf, sizeof(buf), &addr);

            if (bytes == SockErr_WouldBlock)
                break;
            else if (bytes == SockErr_ConnRefused)
            {
                if (m_status == Status::Connecting)
                    SetStatus(Status::FailedToConnect);
                else
                    SetStatus(Status::Disconnected);

                break;
            }

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

            if (m_status == Status::Connecting)
                SetStatus(Status::Connected);

            if (receiveData.callback && m_dataReceiveCallback)
                m_dataReceiveCallback(receiveData.data, receiveData.bytes, receiveData.msgType);
        }

        if (m_status == Status::Connecting &&
            Clock::now() - m_connReqTime > std::chrono::seconds(3))
        {
            SetStatus(Status::FailedToConnect);
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

