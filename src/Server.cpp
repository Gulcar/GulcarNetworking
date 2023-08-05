#include <GulcarNet/Server.h>
#include <GulcarNet/Socket.h>
#include <cassert>

#ifndef GULCAR_NET_RECV_BUF_SIZE
#define GULCAR_NET_RECV_BUF_SIZE 512
#endif

namespace GulcarNet
{
    void Server::Start(uint16_t port)
    {
        if (m_socket)
            m_socket->Close();

        m_socket = std::make_unique<Socket>();
        m_socket->SetBlocking(false);
        m_socket->Bind(port);

        m_socketOpen = true;
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

            if (bytes <= 0)
                continue;

            buf[bytes] = '\0';

            if (m_dataReceiveCallback)
                m_dataReceiveCallback(buf, bytes, connIt->second);
        }
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
