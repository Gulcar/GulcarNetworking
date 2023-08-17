#include <GulcarNet/Server.h>
#include "Socket.h"
#include "Transport.h"
#include <cassert>

#ifndef GULCAR_NET_RECV_BUF_SIZE
#define GULCAR_NET_RECV_BUF_SIZE 512
#endif

namespace Net
{
    Connection::Connection(Socket* socket, IPAddr addr)
    {
        m_addr = addr;
        m_transport = std::make_unique<Transport>(socket, m_addr);
    }

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
    }

    void Server::Stop()
    {
        assert(m_socket && "GulcarNet: Server Start not called!");

        m_socket->Close();
        m_socket = nullptr;
    }

    void Server::SendTo(Buf buf, uint16_t msgType, SendType reliable, Connection& conn)
    {
        assert(m_socket && "GulcarNet: Server Start not called!");
        assert(buf.bytes < GULCAR_NET_RECV_BUF_SIZE && "Cannot send this much data at once!");

        m_stats.AddPacketSent(buf.bytes);
        conn.m_transport->Send(buf.data, buf.bytes, msgType, reliable);
    }

    void Server::SendToAll(Buf buf, uint16_t msgType, SendType reliable)
    {
        for (auto& connIt : m_connections)
        {
            SendTo(buf, msgType, reliable, connIt.second);
        }
    }

    void Server::Process()
    {
        assert(m_socket && "GulcarNet: Server Start not called!");

        for (auto it = m_connections.begin(); it != m_connections.end(); it++)
            it->second.m_transport->SendExtraAcks();

        char buf[GULCAR_NET_RECV_BUF_SIZE];

        for (int i = 0; i < 256; i++)
        {
            IPAddr addr;
            int bytes = m_socket->RecvFrom(buf, sizeof(buf), &addr);

            if (bytes == SockErr_WouldBlock)
                break;
            else if (bytes == SockErr_MsgTooLarge)
                continue;

            auto connIt = m_connections.find(addr);

            if (bytes == SockErr_ConnRefused)
            {
                if (connIt != m_connections.end())
                    DisconnectClient(m_connections.find(addr));
                continue;
            }

            if (connIt == m_connections.end())
                connIt = InsertClient(addr);

            if (bytes <= 0)
                continue;

#ifdef GULCAR_NET_SIM_PACKET_LOSS
            if (rand() % 100 < 3)
                continue;
#endif
            buf[bytes] = '\0';

            Connection& conn = connIt->second;
            Transport::ReceiveData receiveData = conn.m_transport->Receive(buf, bytes);

            m_stats.AddPacketReceived(bytes);

            if (receiveData.callback && m_dataReceiveCallback)
                m_dataReceiveCallback(receiveData.data, receiveData.bytes, receiveData.msgType, conn);
        }

        for (auto it = m_connections.begin(); it != m_connections.end(); it++)
            it->second.m_transport->RetrySending();

        for (auto it = m_connections.begin(); it != m_connections.end(); it++)
        {
            if (it->second.m_transport->IsGettingAcks() == false)
            {
                DisconnectClient(it);
                break;
            }
        }

        m_stats.UpdateTime();
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
        Connection conn(m_socket.get(), addr);
        auto res = m_connections.insert(std::make_pair(addr, std::move(conn)));
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
