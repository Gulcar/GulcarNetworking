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

        struct PacketUnreliable
        {
            SendType sendType;
            uint16_t msgType;

            // data ...
        };

        struct PacketUnreliableDiscardOld
        {
            SendType sendType;
            uint16_t msgType;

            uint16_t seqNum;

            // data ...
        };

        struct PacketReliable
        {
            SendType sendType;
            uint16_t msgType;

            uint16_t seqNum;
            uint16_t ackNum;
            uint32_t ackBits;

            // data ...
        };

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

    public:
        Transport(class Socket* socket, IPAddr addr)
            : m_socket(socket), m_addr(addr) { }

        void Send(const void* data, size_t bytes, uint16_t msgType, SendType reliable);

        struct ReceiveData { void* data; size_t bytes; uint16_t msgType; bool callback; };
        ReceiveData Receive(void* buf, size_t bytes);

        void SendExtraAcks();
        void RetrySending();
        bool IsGettingAcks();

        void SendConnectRequest();

    private:
        void SendUnreliable(const void* data, size_t bytes, uint16_t msgType);
        void SendUnreliableDiscardOld(const void* data, size_t bytes, uint16_t msgType);
        void SendReliable(const void* data, size_t bytes, uint16_t msgType);

        ReceiveData ReceiveUnreliable(void* buf, size_t bytes);
        ReceiveData ReceiveUnreliableDiscardOld(void* buf, size_t bytes);
        ReceiveData ReceiveReliable(void* buf, size_t bytes);

        void SendExtraAcksFrom(uint32_t remoteSeq);

        uint32_t GetAckBits();
        uint32_t GetAckBitsFrom(uint32_t remoteSeq);

    private:
        struct Sequences
        {
            uint16_t localSeqNum = 0; // last seqNum sent
            uint16_t remoteSeqNum = 0; // last seqNum received
        };
        // msgType -> seq (for UnreliableDiscardOld)
        std::unordered_map<uint16_t, Sequences> m_seqMap;

        // for all Reliable
        Sequences m_sequences;

        bool m_needToSendAck = false;
        bool m_gettingAcks = true;

        std::bitset<1024> m_receivedBits;
        std::deque<WaitingForAck> m_waitingForAck;

        class Socket* m_socket;
        IPAddr m_addr;
    };
}
