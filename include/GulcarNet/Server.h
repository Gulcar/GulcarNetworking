#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <functional>
#include <stdint.h>
#include <unordered_map>
#include <memory>

namespace Net
{
    class Connection
    {
        friend class Server;

    public:
        void* userData = nullptr;

        inline const IPAddr& GetAddr() const { return m_addr; }

    private:
        Connection(class Socket* socket, IPAddr addr);

        IPAddr m_addr;
        std::unique_ptr<class Transport> m_transport;
    };

    class Server
    {
    public:
        using ClientConnectedCallback = std::function<void(Connection&)>;
        using ClientDisconnectedCallback = std::function<void(Connection&)>;
        using DataReceiveCallback = std::function<void(void* data, size_t bytes, uint16_t msgType, Connection&)>;

        using ConnectionsMap = std::unordered_map<IPAddr, Connection>;

    public:
        Server();
        ~Server();

        void Start(uint16_t port);
        void Stop();

        void SendTo(Buf buf, uint16_t msgType, SendType reliable, Connection& conn);
        void SendToAll(Buf buf, uint16_t msgType, SendType reliable);

        void Process();

        void SetClientConnectedCallback(ClientConnectedCallback callback);
        void SetClientDisconnectedCallback(ClientDisconnectedCallback callback);
        void SetDataReceiveCallback(DataReceiveCallback callback);

        inline int GetNumClients() { return m_connections.size(); }

    private:
        ConnectionsMap::iterator InsertClient(IPAddr addr);
        void DisconnectClient(ConnectionsMap::iterator it);

    private:
        std::unique_ptr<class Socket> m_socket;
        ConnectionsMap m_connections;

        bool m_socketOpen = false;

        ClientConnectedCallback m_clientConnectedCallback;
        ClientDisconnectedCallback m_clientDisconnectedCallback;
        DataReceiveCallback m_dataReceiveCallback;
    };
}
