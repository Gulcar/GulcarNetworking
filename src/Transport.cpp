#include "Transport.h"
#include "Socket.h"
#include <cassert>
#include <iostream> // TODO: ne iostream

#ifndef GULCAR_NET_RECV_BUF_SIZE
#define GULCAR_NET_RECV_BUF_SIZE 512
#endif

namespace Net
{
    using ReceiveData = Transport::ReceiveData;

    void Transport::Send(const void* data, size_t bytes, uint16_t msgType, SendType reliable)
    {
        switch (reliable)
        {
        case SendType::Unreliable:
            SendUnreliable(data, bytes, msgType);
            return;

        case SendType::UnreliableDiscardOld:
            SendUnreliableDiscardOld(data, bytes, msgType);
            return;

        case SendType::Reliable:
            SendReliable(data, bytes, msgType);
            return;
        }

        assert(false && "GulcarNet: SendType doesnt match!");
        return;
    }

    ReceiveData Transport::Receive(void* buf, size_t bytes)
    {
        SendType sendType = *(SendType*)buf;

        if (sendType == SendType::Unreliable && bytes >= sizeof(PacketUnreliable))
            return ReceiveUnreliable(buf, bytes);

        else if (sendType == SendType::UnreliableDiscardOld && bytes >= sizeof(PacketUnreliableDiscardOld))
            return ReceiveUnreliableDiscardOld(buf, bytes);

        else if (sendType == SendType::Reliable && bytes >= sizeof(PacketReliable))
            return ReceiveReliable(buf, bytes);

        std::cout << "invalid packet\n";
        ReceiveData data;
        data.callback = false;
        return data;
    }

    void Transport::SendExtraAcks()
    {
        if (m_needToSendAck == false)
            return;
        m_needToSendAck = false;

        std::cout << "sending extra ack\n";

        PacketReliable packet;
        packet.sendType = SendType::Reliable;
        packet.msgType = MsgType_AcksOnly;
        packet.seqNum = ++m_sequences.localSeqNum;
        packet.ackNum = m_sequences.remoteSeqNum;

        // TODO: v tem filu najdi svtari ki se ponavljajo
        // kot je tole racunanje ackBitov
        packet.ackBits = 0;
        for (int i = 0; i < 32; i++)
        {
            int seq = m_sequences.remoteSeqNum - 1 - i;
            if (seq < 0) break;

            if (m_receivedBits[seq % 1024] == true)
                packet.ackBits |= (1 << i);
        }

        int res = m_socket->SendTo(&packet, sizeof(PacketReliable), m_addr);
        assert(res == sizeof(PacketReliable) && "GulcarNet: failed to send!");
    }

    void Transport::RetrySending()
    {
        while (!m_waitingForAck.empty())
        {
            WaitingForAck& waiting = m_waitingForAck.front();

            if (waiting.acked)
            {
                std::cout << "acked: " << waiting.packet->seqNum << "\n";
                delete[] waiting.packet;
                m_waitingForAck.pop_front();
                break;
            }

            auto duration = Clock::now() - waiting.timeSent;
            if (duration < std::chrono::milliseconds(100))
                break;

            std::cout << "resending: " << waiting.packet->seqNum << "\n";

            waiting.packet->ackNum = m_sequences.remoteSeqNum;

            waiting.packet->ackBits = 0;
            for (int i = 0; i < 32; i++)
            {
                int seq = m_sequences.remoteSeqNum - 1 - i;
                if (seq < 0) break;

                if (m_receivedBits[seq % 1024] == true)
                    waiting.packet->ackBits |= (1 << i);
            }

            int res = m_socket->SendTo(waiting.packet, waiting.packetSize, m_addr);
            assert(res == waiting.packetSize && "GulcarNet: failed to send!");

            m_waitingForAck.push_back({
                Clock::now(),
                waiting.packet,
                waiting.packetSize
            });

            m_waitingForAck.pop_front();
        }
    }

    void Transport::SendUnreliable(const void* data, size_t bytes, uint16_t msgType)
    {
        using Packet = PacketUnreliable;
        char buf[GULCAR_NET_RECV_BUF_SIZE];

        Packet* packet = (Packet*)buf;
        packet->sendType = SendType::Unreliable;
        packet->msgType = msgType;

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        int res = m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
        assert(res == (sizeof(Packet) + bytes) && "GulcarNet: failed to send!");
    }

    void Transport::SendUnreliableDiscardOld(const void* data, size_t bytes, uint16_t msgType)
    {
        using Packet = PacketUnreliableDiscardOld;
        char buf[GULCAR_NET_RECV_BUF_SIZE];

        Packet* packet = (Packet*)buf;
        packet->sendType = SendType::UnreliableDiscardOld;
        packet->msgType = msgType;
        packet->seqNum = ++(m_seqMap[msgType].localSeqNum);

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        int res = m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
        assert(res == (sizeof(Packet) + bytes) && "GulcarNet: failed to send!");
    }

    void Transport::SendReliable(const void* data, size_t bytes, uint16_t msgType)
    {
        using Packet = PacketReliable;

        Packet* packet = (Packet*)(new char[sizeof(Packet) + bytes]);
        packet->sendType = SendType::Reliable;
        packet->msgType = msgType;
        packet->seqNum = ++m_sequences.localSeqNum;
        packet->ackNum = m_sequences.remoteSeqNum;

        packet->ackBits = 0;
        for (int i = 0; i < 32; i++)
        {
            int seq = m_sequences.remoteSeqNum - 1 - i;
            if (seq < 0) break;

            if (m_receivedBits[seq % 1024] == true)
                packet->ackBits |= (1 << i);
        }

        m_needToSendAck = false;

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        m_waitingForAck.push_back({
            Clock::now(),
            packet,
            sizeof(Packet) + bytes
        });

        int res = m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
        assert(res == (sizeof(Packet) + bytes) && "GulcarNet: failed to send!");
    }

    
    ReceiveData Transport::ReceiveUnreliable(void* buf, size_t bytes)
    {
        PacketUnreliable* packet = (PacketUnreliable*)buf;
        std::cout << "received unreliable packet\n";
        std::cout << "msgType: " << packet->msgType << "\n";

        ReceiveData data;
        data.data = (char*)buf + sizeof(PacketUnreliable);
        data.bytes = bytes - sizeof(PacketUnreliable);
        data.msgType = packet->msgType;
        data.callback = true;
        return data;
    }

    ReceiveData Transport::ReceiveUnreliableDiscardOld(void* buf, size_t bytes)
    {
        PacketUnreliableDiscardOld* packet = (PacketUnreliableDiscardOld*)buf;
        std::cout << "received unreliable discard old packet\n";
        std::cout << "msgType: " << packet->msgType << "\n";
        std::cout << "seqNum: " << packet->seqNum << "\n";

        uint16_t& remoteSeq = m_seqMap[packet->msgType].remoteSeqNum;

        ReceiveData data;

        int diff = (int)packet->seqNum - (int)remoteSeq;
        if ((diff > 0 && diff < 16.384) || (diff < -49.152))
        {
            remoteSeq = packet->seqNum;

            data.data = (char*)buf + sizeof(PacketUnreliableDiscardOld);
            data.bytes = bytes - sizeof(PacketUnreliableDiscardOld);
            data.msgType = packet->msgType;
            data.callback = true;
        }
        else
        {
            std::cout << "packet discarded\n";
            data.callback = false;
        }

        return data;
    }

    ReceiveData Transport::ReceiveReliable(void* buf, size_t bytes)
    {
        PacketReliable* packet = (PacketReliable*)buf;
        std::cout << "received reliable packet\n";
        std::cout << "msgType: " << packet->msgType << "\n";
        std::cout << "seqNum: " << packet->seqNum << "\n";
        std::cout << "ackNum: " << packet->ackNum << "\n";
        std::cout << "ackBits: " << std::bitset<32>(packet->ackBits) << "\n";

        int diff = (int)packet->seqNum - (int)m_sequences.remoteSeqNum;
        if ((diff > 0 && diff < 16.384) || (diff < -49.152))
        {
            // izbrisi m_receivedBits ki jih ne bomo vec posiljali kot ack
            int from = std::max(m_sequences.remoteSeqNum - 32, 0);
            int to = packet->seqNum - 32;
            for (int i = from; i < to; i++)
                m_receivedBits[i % 1024] = false;

            m_sequences.remoteSeqNum = packet->seqNum;
        }

        if (m_receivedBits[packet->seqNum % 1024] == true)
        {
            std::cout << "already received packet " << packet->seqNum << "\n";
            m_needToSendAck = true;
            ReceiveData data;
            data.callback = false;
            return data;
        }

        m_receivedBits[packet->seqNum % 1024] = true;

        if (packet->msgType != MsgType_AcksOnly)
            m_needToSendAck = true;

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

        ReceiveData data;
        data.data = (char*)buf + sizeof(PacketReliable);
        data.bytes = bytes - sizeof(PacketReliable);
        data.msgType = packet->msgType;
        data.callback = true;

        if (packet->msgType == MsgType_AcksOnly)
            data.callback = false;

        return data;
    }
}