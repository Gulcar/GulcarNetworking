#pragma once

#include <GulcarNet/IPAddr.h>
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
        IPAddr m_addr;
        uint32_t m_seqNum = 0;
    };

    class Server
    {
    public:
        using ClientConnectedCallback = std::function<void(Connection&)>;
        using ClientDisconnectedCallback = std::function<void(Connection&)>;
        using DataReceiveCallback = std::function<void(void* data, size_t bytes, Connection&)>;
        using ConnectionsMap = std::unordered_map<IPAddr, Connection>;

    public:
        Server();
        ~Server();

        void Start(uint16_t port);
        void Stop();

        void SendTo(const void* data, size_t bytes, Connection& conn);

        template<typename T>
        void SendTo(const T& data, Connection& conn)
        {
            SendTo(&data, sizeof(T), conn);
        }

        void SendToAll(const void* data, size_t bytes);

        template<typename T>
        void SendToAll(const T& data)
        {
            SendToAll(&data, sizeof(T));
        }

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
