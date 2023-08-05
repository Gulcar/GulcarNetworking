#include <GulcarNet/Server.h>
#include "Socket.h"
#include "Packet.h"
#include <cassert>
#include <iostream> // TODO: ne includaj iostream

#ifndef GULCAR_NET_RECV_BUF_SIZE
#define GULCAR_NET_RECV_BUF_SIZE 512
#endif

namespace Net
{
    Server::Server()
    {
        Net::InitSockets();
    }

    Server::~Server()
    {
        Net::ShutdownSockets();
    }

    void Server::Start(uint16_t port)
    {
        if (m_socket)
            m_socket->Close();

        m_socket = std::make_unique<Socket>();
        m_socket->SetBlocking(false);
        m_socket->Bind(port);

        m_socketOpen = true;

        // default channels
        if (m_channels.empty())
        {
            m_channels.emplace_back(ChannelType::Unreliable);
            m_channels.emplace_back(ChannelType::Reliable);
        }
    }

    void Server::Stop()
    {
        assert(m_socketOpen && "GulcarNet: Server Start not called!");

        m_socket->Close();
        m_socketOpen = false;
    }

    void Server::SendTo(const void* data, size_t bytes, Connection& conn)
    {
        assert(m_socketOpen && "GulcarNet: Server Start not called!");

        // TODO: nared neki z intom ki ga dobis tukaj nazaj (poglej tudi v client send)
        m_socket->SendTo(data, bytes, conn.GetAddr());
    }

    void Server::SendToAll(const void* data, size_t bytes)
    {
        for (auto& connIt : m_connections)
        {
            SendTo(data, bytes, connIt.second);
        }
    }

    void Server::Process()
    {
        assert(m_socketOpen && "GulcarNet: Server Start not called!");

        char buf[GULCAR_NET_RECV_BUF_SIZE];

        for (int i = 0; i < 256; i++)
        {
            IPAddr addr;
            int bytes = m_socket->RecvFrom(buf, sizeof(buf), &addr);

            if (bytes == SockErr_WouldBlock)
                break;

            auto& connIt = m_connections.find(addr);
            if (connIt == m_connections.end())
                connIt = InsertClient(addr);

            if (bytes == SockErr_ConnRefused)
                DisconnectClient(connIt);

            if (bytes < 2)
                continue;

            buf[bytes] = '\0';

            uint16_t channelId = *(uint16_t*)buf;
            if (channelId >= m_channels.size())
                continue;
            Channel& channel = m_channels[channelId];

            switch (channel.chType)
            {
            case ChannelType::Unreliable:
            {
                PacketUnreliable* packet = (PacketUnreliable*)buf;
                std::cout << "received unreliable packet\n";
                std::cout << "channelId: " << packet->channelId << "\n";
                std::cout << "type: " << packet->type << "\n";
                if (m_dataReceiveCallback)
                    m_dataReceiveCallback(buf + sizeof(PacketUnreliable), bytes - sizeof(PacketUnreliable), connIt->second);
                break;
            }
            case ChannelType::UnreliableDiscardOld:
            {
                PacketUnreliableDiscardOld* packet = (PacketUnreliableDiscardOld*)buf;
                std::cout << "received unreliable discard old packet\n";
                std::cout << "channelId: " << packet->channelId << "\n";
                std::cout << "type: " << packet->type << "\n";
                std::cout << "seqNum: " << packet->seqNum << "\n";
                if (m_dataReceiveCallback)
                    m_dataReceiveCallback(buf + sizeof(PacketUnreliableDiscardOld), bytes - sizeof(PacketUnreliableDiscardOld), connIt->second);
                break;
            }
            case ChannelType::Reliable:
            {
                PacketReliable* packet = (PacketReliable*)buf;
                std::cout << "received reliable packet\n";
                std::cout << "channelId: " << packet->channelId << "\n";
                std::cout << "type: " << packet->type << "\n";
                std::cout << "seqNum: " << packet->seqNum << "\n";
                std::cout << "ackNum: " << packet->ackNum << "\n";
                std::cout << "ackBits: " << packet->ackBits << "\n";
                if (m_dataReceiveCallback)
                    m_dataReceiveCallback(buf + sizeof(PacketUnreliableDiscardOld), bytes - sizeof(PacketUnreliableDiscardOld), connIt->second);
                break;
            }
            }
        }
    }

    void Server::SetChannel(uint16_t id, ChannelType type)
    {
        if (m_channels.size() <= id)
            m_channels.resize(id + 1);

        m_channels[id] = Channel(type);
    }

    void Server::SetClientConnectedCallback(ClientConnectedCallback callback)
    {
        m_clientConnectedCallback = callback;
    }

    void Server::SetClientDisconnectedCallback(ClientDisconnectedCallback callback)
    {
        m_clientDisconnectedCallback = callback;
    }

    void Server::SetDataReceiveCallback(DataReceiveCallback callback)
    {
        m_dataReceiveCallback = callback;
    }

    Server::ConnectionsMap::iterator Server::InsertClient(IPAddr addr)
    {
        Connection conn;
        conn.m_addr = addr;
        auto res = m_connections.insert({ addr, conn });
        auto it = res.first;

        if (m_clientConnectedCallback)
            m_clientConnectedCallback(it->second);

        return res.first;
    }

    void Server::DisconnectClient(ConnectionsMap::iterator it)
    {
        if (m_clientDisconnectedCallback)
            m_clientDisconnectedCallback(it->second);

        m_connections.erase(it);
    }
}
