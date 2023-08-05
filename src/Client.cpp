#include <GulcarNet/Client.h>
#include "Socket.h"
#include "Packet.h"
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

        // default channels
        if (m_channels.empty())
        {
            m_channels.emplace_back(ChannelType::Unreliable);
            m_channels.emplace_back(ChannelType::Reliable);
        }
    }

    void Client::Disconnect()
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");

        m_socket->Close();

        SetStatus(Status::Disconnected);
        // TODO: poslji serverju disconnect
    }

    int Client::Send(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType)
    {
        assert(m_socketOpen && "GulcarNet: Client Connect not called!");

        Channel& channel = m_channels[channelId];

        switch (channel.chType)
        {
        case ChannelType::Unreliable:
            return SendUnreliable(data, bytes, channelId, msgType);

        case ChannelType::UnreliableDiscardOld:
            return SendUnreliableDiscardOld(data, bytes, channelId, msgType);

        case ChannelType::Reliable:
            return SendReliable(data, bytes, channelId, msgType);
        }

        assert(false && "GulcarNet: ChannelType doest match!");
        return -1;
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

    void Client::SetChannel(uint16_t id, ChannelType type)
    {
        if (m_channels.size() <= id)
            m_channels.resize(id + 1);

        m_channels[id] = Channel(type);
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

    int Client::SendUnreliable(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType)
    {
        using Packet = PacketUnreliable;
        char buf[GULCAR_NET_RECV_BUF_SIZE];

        Packet* packet = (Packet*)buf;
        packet->channelId = channelId;
        packet->type = msgType;

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_serverAddr);
    }

    int Client::SendUnreliableDiscardOld(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType)
    {
        using Packet = PacketUnreliableDiscardOld;
        char buf[GULCAR_NET_RECV_BUF_SIZE];
        Channel& channel = m_channels[channelId];

        Packet* packet = (Packet*)buf;
        packet->channelId = channelId;
        packet->type = msgType;
        packet->seqNum = channel.localSeqNum++;

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_serverAddr);
    }

    int Client::SendReliable(const void* data, size_t bytes, uint16_t channelId, uint16_t msgType)
    {
        using Packet = PacketReliable;
        Channel& channel = m_channels[channelId];

        Packet* packet = (Packet*)(new char[sizeof(Packet) + bytes]);
        packet->channelId = channelId;
        packet->type = msgType;
        packet->seqNum = channel.localSeqNum++;
        packet->ackNum = channel.remoteSeqNum;
        packet->ackBits = 0; // TODO

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        channel.waitingForAck.push({
            Clock::now(),
            packet
        });

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_serverAddr);
    }
}

