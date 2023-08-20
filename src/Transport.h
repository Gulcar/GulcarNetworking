#pragma once

#include <GulcarNet/IPAddr.h>
#include <GulcarNet/Other.h>
#include <stdint.h>
#include <bitset>
#include <deque>
#include <chrono>
#include <unordered_map>

namespace Net
{
    // handles sending and receving packets with reliability
    class Transport
    {
        using Clock = std::chrono::steady_clock;

#pragma pack(push, 1)
        struct PacketUnreliable
        {
            uint16_t protocolId;
            SendType sendType;

            // (u16 msgType, u16 size, data) ...
        };

        struct PacketUnreliableDiscardOld
        {
            uint16_t protocolId;
            SendType sendType;

            uint16_t seqNum;

            // (u16 msgType, u16 size, data) ...
        };

        struct PacketReliable
        {
            uint16_t protocolId;
            SendType sendType;

            uint16_t seqNum;
            uint16_t ackNum;
            uint32_t ackBits;

            // (u16 msgType, u16 size, data) ...
        };
#pragma pack(pop)

        struct WaitingForAck
        {
            Clock::time_point timeSent;
            PacketReliable* packet;
            size_t packetSize; // total size (header + body)
            int resendCount = 0;
            bool acked = false;
        };

        enum ReservedMsgType : uint16_t
        {
            MsgType_AcksOnly = 65000, // no data (used by SendExtraAcks)
            MsgType_ConnectRequest, // no data (used by SendConnectRequest)
        };

        // za poslijanje vec paketov na enkrat
        struct SendBuf
        {
            char data[GULCAR_NET_RECV_BUF_SIZE];
            int bytes;

            SendBuf(int bytes) : bytes(bytes) {}
        };

    public:
        Transport(class Socket* socket, IPAddr addr)
            : m_socket(socket), m_addr(addr) { }

        void QueueSend(const void* data, size_t bytes, uint16_t msgType, SendType reliable);

        using ReceiveDataCallback = std::function<void(void* data, size_t bytes, uint16_t msgType)>;
        void Receive(void* buf, size_t bytes, ReceiveDataCallback callback);

        void FlushSendQueue();
        void SendExtraAcks();
        void RetrySending();
        bool IsGettingAcks();

        void SendConnectRequest();

    private:
        void SendUnreliable();
        void SendUnreliableDiscardOld();
        void SendReliable();

        void ReceiveUnreliable(void* buf, size_t bytes, ReceiveDataCallback callback);
        void ReceiveUnreliableDiscardOld(void* buf, size_t bytes, ReceiveDataCallback callback);
        void ReceiveReliable(void* buf, size_t bytes, ReceiveDataCallback callback);

        void SendExtraAcksFrom(uint16_t remoteSeq);

        uint32_t GetAckBits();
        uint32_t GetAckBitsFrom(uint16_t remoteSeq);

    private:
        struct Sequences
        {
            uint16_t localSeqNum = 0; // last seqNum sent
            uint16_t remoteSeqNum = 0; // last seqNum received
        };
        // msgType -> seq (for UnreliableDiscardOld)
        std::unordered_map<uint16_t, uint16_t> m_seqMap;
        uint16_t m_localSeqUnreliableDiscardOld = 0;

        // for all Reliable
        Sequences m_sequences;

        bool m_needToSendAck = false;
        bool m_gettingAcks = true;

        std::bitset<1024> m_receivedBits;
        std::deque<WaitingForAck> m_waitingForAck;

        SendBuf m_unreliableSendBuf = SendBuf(sizeof(PacketUnreliable));
        SendBuf m_unreliableDiscardOldSendBuf = SendBuf(sizeof(PacketUnreliableDiscardOld));
        SendBuf m_reliableSendBuf = SendBuf(sizeof(PacketReliable));

        class Socket* m_socket;
        IPAddr m_addr;
    };
}
