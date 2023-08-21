#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <functional>
#include <stdint.h>
#include <unordered_map>
#include <memory>

namespace Net
{
    /** 
    * Represents the client that is connected to the Server.
    * 
    * Instances of this class will only be made by the Server.
    */
    class Connection
    {
        friend class Server;

    public:
        /** set this to anything you want the Connection to point to */
        void* userData = nullptr;
        /** returns the client ip address */
        inline const IPAddr& GetAddr() const { return m_addr; }

    private:
        Connection(class Socket* socket, IPAddr addr, Statistics* stats);

        IPAddr m_addr;
        std::unique_ptr<class Transport> m_transport;
    };

    /** network interface for the server */
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

        /** Starts listening on the specified port. */
        void Start(uint16_t port);
        /** Shut down the server. */
        void Stop();

        /** Sends a message to a specific Connection */
        void SendTo(Buf buf, uint16_t msgType, SendType reliable, Connection& conn);
        /** Sends a message to all current connections */
        void SendToAll(Buf buf, uint16_t msgType, SendType reliable);

        /**
        * Call this every frame.
        * 
        * It will receive data on the socket and manage acks.
        * Your callbacks will be called in this function.
        */
        void Process();

        /* Sets the callback which will be called when a client connects. Called inside Process(). */
        void SetClientConnectedCallback(ClientConnectedCallback callback);
        /* Sets the callback which will be called when a client disconnects. Called inside Process(). */
        void SetClientDisconnectedCallback(ClientDisconnectedCallback callback);
        /* Sets the callback which will be called when a client message is received. Called inside Process(). */
        void SetDataReceiveCallback(DataReceiveCallback callback);

        /** gets the current number of clients */
        inline int GetNumClients() { return m_connections.size(); }

        /** gets the unordered_map begin iterator for current connections. */
        inline ConnectionsMap::iterator GetClientIterBegin() { return m_connections.begin(); }
        /** gets the unordered_map end iterator for current connections. */
        inline ConnectionsMap::iterator GetClientIterEnd() { return m_connections.end(); }

        /** Returns the networking statistics (bytes sent, bytes received, ...) */
        inline const Statistics& GetStats() const { return m_stats; }

    private:
        ConnectionsMap::iterator InsertClient(IPAddr addr);
        void DisconnectClient(ConnectionsMap::iterator it);

    private:
        std::unique_ptr<class Socket> m_socket;
        ConnectionsMap m_connections;

        ClientConnectedCallback m_clientConnectedCallback;
        ClientDisconnectedCallback m_clientDisconnectedCallback;
        DataReceiveCallback m_dataReceiveCallback;

        Statistics m_stats;
    };
}
