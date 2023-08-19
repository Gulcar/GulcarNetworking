#include "Transport.h"
#include "Socket.h"
#include <cassert>
#include <unordered_set>

#ifdef GULCAR_NET_DEBUG_OUT
    #include <iostream>
    #define DEBUG(x) std::cout << x
#else
    #define DEBUG(x)
#endif

#ifndef GULCAR_NET_PROTOCOL_ID
#define GULCAR_NET_PROTOCOL_ID 0x0504
#endif

namespace Net
{
    constexpr auto timeToResend = std::chrono::milliseconds(100);
    constexpr int maxResendCount = std::chrono::seconds(5) / timeToResend;

    void Transport::QueueSend(const void* data, size_t bytes, uint16_t msgType, SendType reliable)
    {
        SendBuf* buf;

        switch (reliable)
        {
        case SendType::Unreliable:
        {
            buf = &m_unreliableSendBuf;
            if (buf->bytes + bytes + 4 >= GULCAR_NET_RECV_BUF_SIZE)
                SendUnreliable();
            break;
        }
        case SendType::UnreliableDiscardOld:
        {
            buf = &m_unreliableDiscardOldSendBuf;
            if (buf->bytes + bytes + 4 >= GULCAR_NET_RECV_BUF_SIZE)
                SendUnreliableDiscardOld();
            break;
        }
        case SendType::Reliable:
        {
            buf = &m_reliableSendBuf;
            if (buf->bytes + bytes + 4 >= GULCAR_NET_RECV_BUF_SIZE)
                SendReliable();
            break;
        }
        default: assert(false && "invalid send type\n");
        }

        uint16_t size = bytes;

        memcpy(buf->data + buf->bytes, &msgType, 2);
        memcpy(buf->data + buf->bytes + 2, &size, 2);
        memcpy(buf->data + buf->bytes + 4, data, bytes);

        buf->bytes += 4 + bytes;

        DEBUG("send queued\n");
    }

    void Transport::Receive(void* buf, size_t bytes, ReceiveDataCallback callback)
    {
        PacketUnreliable packet = *(PacketUnreliable*)buf;

        if (packet.protocolId != GULCAR_NET_PROTOCOL_ID)
        {
            DEBUG("invalid packet protocol id: " << std::hex << packet.protocolId << "\n");
            return;
        }

        if (packet.sendType == SendType::Unreliable && bytes >= sizeof(PacketUnreliable))
            return ReceiveUnreliable(buf, bytes, callback);

        else if (packet.sendType == SendType::UnreliableDiscardOld && bytes >= sizeof(PacketUnreliableDiscardOld))
            return ReceiveUnreliableDiscardOld(buf, bytes, callback);

        else if (packet.sendType == SendType::Reliable && bytes >= sizeof(PacketReliable))
            return ReceiveReliable(buf, bytes, callback);

        DEBUG("invalid packet send type\n");
        return;
    }

    void Transport::FlushSendQueue()
    {
        if (m_unreliableSendBuf.bytes > sizeof(PacketUnreliable))
            SendUnreliable();
        if (m_unreliableDiscardOldSendBuf.bytes > sizeof(PacketUnreliableDiscardOld))
            SendUnreliableDiscardOld();
        if (m_reliableSendBuf.bytes > sizeof(PacketReliable))
            SendReliable();
    }

    void Transport::SendExtraAcks()
    {
        if (m_needToSendAck == false)
            return;
        m_needToSendAck = false;

        SendExtraAcksFrom(m_sequences.remoteSeqNum);
    }

    void Transport::RetrySending()
    {
        while (!m_waitingForAck.empty())
        {
            WaitingForAck& waiting = m_waitingForAck.front();

            if (waiting.acked)
            {
                DEBUG("acked: " << waiting.packet->seqNum << "\n");
                delete[] waiting.packet;
                m_waitingForAck.pop_front();
                break;
            }

            auto duration = Clock::now() - waiting.timeSent;
            // TODO: cas med resendanjem bi moral postajati vedno vecji da ne preobremenis networka
            if (duration < std::chrono::milliseconds(100))
                break;

            // ce ne predolgo nismo dobili acka
            if (waiting.resendCount >= maxResendCount)
            {
                m_gettingAcks = false;
                return;
            }

            DEBUG("resending: " << waiting.packet->seqNum << "\n");

            waiting.packet->ackNum = m_sequences.remoteSeqNum;
            waiting.packet->ackBits = GetAckBits();

            int res = m_socket->SendTo(waiting.packet, waiting.packetSize, m_addr);
            assert(res == waiting.packetSize && "GulcarNet: failed to send!");

            m_waitingForAck.push_back({
                Clock::now(),
                waiting.packet,
                waiting.packetSize,
                waiting.resendCount + 1
            });

            m_waitingForAck.pop_front();
        }
    }

    bool Transport::IsGettingAcks()
    {
        return m_gettingAcks;
    }

    void Transport::SendConnectRequest()
    {
        QueueSend(nullptr, 0, MsgType_ConnectRequest, SendType::Reliable);
    }

    void Transport::SendUnreliable()
    {
        using Packet = PacketUnreliable;

        Packet* packet = (Packet*)m_unreliableSendBuf.data;
        packet->protocolId = GULCAR_NET_PROTOCOL_ID;
        packet->sendType = SendType::Unreliable;

        int res = m_socket->SendTo(packet, m_unreliableSendBuf.bytes, m_addr);
        assert(res == m_unreliableSendBuf.bytes && "GulcarNet: failed to send!");

        DEBUG("send unreliable " << m_unreliableSendBuf.bytes << " bytes\n");

        m_unreliableSendBuf.bytes = sizeof(Packet);
    }

    void Transport::SendUnreliableDiscardOld()
    {
        using Packet = PacketUnreliableDiscardOld;

        Packet* packet = (Packet*)m_unreliableDiscardOldSendBuf.data;
        packet->protocolId = GULCAR_NET_PROTOCOL_ID;
        packet->sendType = SendType::UnreliableDiscardOld;
        packet->seqNum = ++m_localSeqUnreliableDiscardOld;

        int res = m_socket->SendTo(packet, m_unreliableDiscardOldSendBuf.bytes, m_addr);
        assert(res == m_unreliableDiscardOldSendBuf.bytes && "GulcarNet: failed to send!");

        DEBUG("send unreliable discard old " << m_unreliableDiscardOldSendBuf.bytes << " bytes\n");

        m_unreliableDiscardOldSendBuf.bytes = sizeof(Packet);
    }

    void Transport::SendReliable()
    {
        using Packet = PacketReliable;

        Packet* packet = (Packet*)(new char[m_reliableSendBuf.bytes]);
        packet->protocolId = GULCAR_NET_PROTOCOL_ID;
        packet->sendType = SendType::Reliable;
        packet->seqNum = ++m_sequences.localSeqNum;
        packet->ackNum = m_sequences.remoteSeqNum;
        packet->ackBits = GetAckBits();

        m_needToSendAck = false;

        memcpy((char*)packet + sizeof(Packet), m_reliableSendBuf.data + sizeof(Packet), m_reliableSendBuf.bytes - sizeof(Packet));

        m_waitingForAck.push_back({
            Clock::now(),
            packet,
            (size_t)m_reliableSendBuf.bytes
        });

        int res = m_socket->SendTo(packet, m_reliableSendBuf.bytes, m_addr);
        assert(res == m_reliableSendBuf.bytes && "GulcarNet: failed to send!");

        DEBUG("send reliable " << m_reliableSendBuf.bytes << " bytes\n");

        m_reliableSendBuf.bytes = sizeof(Packet);
    }

    
    void Transport::ReceiveUnreliable(void* buf, size_t bytes, Transport::ReceiveDataCallback callback)
    {
        PacketUnreliable* packet = (PacketUnreliable*)buf;
        DEBUG("received unreliable packet\n");

        int i = sizeof(PacketUnreliable);
        while (i < bytes)
        {
            uint16_t msgType = *(uint16_t*)((char*)buf + i);
            uint16_t size = *(uint16_t*)((char*)buf + i + 2);

            if (i + 4 + size > bytes)
            {
                DEBUG("invalid packet size\n");
                return;
            }

            callback((char*)buf + i + 4, size, msgType);

            i += 4 + size;
        }
    }

    void Transport::ReceiveUnreliableDiscardOld(void* buf, size_t bytes, Transport::ReceiveDataCallback callback)
    {
        PacketUnreliableDiscardOld* packet = (PacketUnreliableDiscardOld*)buf;
        DEBUG("received unreliable discard old packet\n");
        DEBUG("seqNum: " << packet->seqNum << "\n");

        static std::unordered_set<uint16_t> seqUpdatesThisPacket;
        seqUpdatesThisPacket.clear();

        int i = sizeof(PacketUnreliableDiscardOld);
        while (i < bytes)
        {
            uint16_t msgType = *(uint16_t*)((char*)buf + i);
            uint16_t size = *(uint16_t*)((char*)buf + i + 2);

            if (i + 4 + size > bytes)
            {
                DEBUG("invalid packet size\n");
                return;
            }

            bool alreadyUpdatedSeq = seqUpdatesThisPacket.find(msgType) != seqUpdatesThisPacket.end();
            if (alreadyUpdatedSeq)
            {
                callback((char*)buf + i + 4, size, msgType);
            }
            else
            {
                uint16_t& remoteSeq = m_seqMap[msgType];

                int diff = (int)packet->seqNum - (int)remoteSeq;
                if ((diff > 0 && diff < 16.384) || (diff < -49.152))
                {
                    remoteSeq = packet->seqNum;
                    seqUpdatesThisPacket.insert(msgType);
                    callback((char*)buf + i + 4, size, msgType);
                }
                else
                {
                    DEBUG("packet discarded\n");
                }
            }

            i += 4 + size;
        }
    }

    void Transport::ReceiveReliable(void* buf, size_t bytes, Transport::ReceiveDataCallback callback)
    {
        PacketReliable* packet = (PacketReliable*)buf;
        DEBUG("received reliable packet\n");
        DEBUG("seqNum: " << packet->seqNum << "\n");
        DEBUG("ackNum: " << packet->ackNum << "\n");
        DEBUG("ackBits: " << std::bitset<32>(packet->ackBits) << "\n");

        int diff = (int)packet->seqNum - (int)m_sequences.remoteSeqNum;
        if ((diff > 0 && diff < 16.384) || (diff < -49.152))
        {
            // TODO: tole ni kul (kaj se zgodi ko wrappa in kaj ko posiljamo acke za nazaj ce jih tu resetiramo)
            // izbrisi m_receivedBits ki jih ne bomo vec posiljali kot ack
            int from = std::max(m_sequences.remoteSeqNum - 32, 0);
            int to = packet->seqNum - 32;
            for (int i = from; i < to; i++)
                m_receivedBits[i % 1024] = false;

            m_sequences.remoteSeqNum = packet->seqNum;
        }

        if (m_receivedBits[packet->seqNum % 1024] == true)
        {
            DEBUG("already received packet " << packet->seqNum << "\n");

            if (m_sequences.remoteSeqNum - packet->seqNum > 30)
            {
                SendExtraAcksFrom(packet->seqNum);
            }
            else
            {
                m_needToSendAck = true;
            }
            
            return;
        }

        m_receivedBits[packet->seqNum % 1024] = true;

        for (auto it = m_waitingForAck.begin(); it != m_waitingForAck.end(); it++)
        {
            if (it->acked)
                continue;

            if (it->packet->seqNum == packet->ackNum)
            {
                it->acked = true;
                continue;
            }

            int diff = (int)packet->ackNum - (int)it->packet->seqNum;
            if (diff < 0 || diff >= 32) continue;

            it->acked = (packet->ackBits >> (diff-1)) & 1;
        }

        int i = sizeof(PacketReliable);
        while (i < bytes)
        {
            uint16_t msgType = *(uint16_t*)((char*)buf + i);
            uint16_t size = *(uint16_t*)((char*)buf + i + 2);

            if (i + 4 + size > bytes)
            {
                DEBUG("invalid packet size: " << size << "\n");
                return;
            }

            if (msgType != MsgType_AcksOnly)
                m_needToSendAck = true;
            
            if (msgType != MsgType_AcksOnly &&
                msgType != MsgType_ConnectRequest)
            {
                callback((char*)buf + i + 4, size, msgType);
            }

            i += 4 + size;
        }
    }

    void Transport::SendExtraAcksFrom(uint32_t remoteSeq)
    {
        DEBUG("sending extra ack\n");

        char buf[sizeof(PacketReliable) + 4];
        PacketReliable* packet = (PacketReliable*)buf;
        packet->protocolId = GULCAR_NET_PROTOCOL_ID;
        packet->sendType = SendType::Reliable;
        packet->seqNum = ++m_sequences.localSeqNum;
        packet->ackNum = remoteSeq;
        packet->ackBits = GetAckBitsFrom(remoteSeq);

        uint16_t msgType = MsgType_AcksOnly;
        memcpy(buf + sizeof(PacketReliable), &msgType, 2);
        memset(buf + sizeof(PacketReliable) + 2, 0, 2);

        int res = m_socket->SendTo(packet, sizeof(PacketReliable), m_addr);
        assert(res == sizeof(PacketReliable) && "GulcarNet: failed to send!");
    }

    uint32_t Transport::GetAckBits()
    {
        return GetAckBitsFrom(m_sequences.remoteSeqNum);
    }

    uint32_t Transport::GetAckBitsFrom(uint32_t remoteSeq)
    {
        uint32_t bits = 0;

        for (int i = 0; i < 32; i++)
        {
            int seq = remoteSeq - 1 - i;
            if (seq < 0) break;

            if (m_receivedBits[seq % 1024] == true)
                bits |= (1 << i);
        }

        return bits;
    }
}
