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

    int Transport::Send(const void* data, size_t bytes, uint16_t msgType, SendType reliable)
    {
        switch (reliable)
        {
        case SendType::Unreliable:
            return SendUnreliable(data, bytes, msgType);

        case SendType::UnreliableDiscardOld:
            return SendUnreliableDiscardOld(data, bytes, msgType);

        case SendType::Reliable:
            return SendReliable(data, bytes, msgType);
        }

        assert(false && "GulcarNet: SendType doesnt match!");
        return -1;
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

    int Transport::SendUnreliable(const void* data, size_t bytes, uint16_t msgType)
    {
        using Packet = PacketUnreliable;
        char buf[GULCAR_NET_RECV_BUF_SIZE];

        Packet* packet = (Packet*)buf;
        packet->sendType = SendType::Unreliable;
        packet->msgType = msgType;

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
    }

    int Transport::SendUnreliableDiscardOld(const void* data, size_t bytes, uint16_t msgType)
    {
        using Packet = PacketUnreliableDiscardOld;
        char buf[GULCAR_NET_RECV_BUF_SIZE];

        Packet* packet = (Packet*)buf;
        packet->sendType = SendType::UnreliableDiscardOld;
        packet->msgType = msgType;
        packet->seqNum = ++(m_seqMap[msgType].localSeqNum);

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
    }

    int Transport::SendReliable(const void* data, size_t bytes, uint16_t msgType)
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

        memcpy((char*)packet + sizeof(Packet), data, bytes);

        m_waitingForAck.push({
            Clock::now(),
            packet
        });

        return m_socket->SendTo(packet, sizeof(Packet) + bytes, m_addr);
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
        if ((diff > 0 && diff < 16.384) || (diff < 49.152))
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
        if ((diff > 0 && diff < 16.384) || (diff < 49.152))
        {
            // izbrisi m_receivedBits ki jih ne bomo vec posiljali kot ack
            int from = std::max(m_sequences.remoteSeqNum - 32, 0);
            int to = packet->seqNum - 32;
            for (int i = from; i < to; i++)
                m_receivedBits[i % 1024] = false;

            m_sequences.remoteSeqNum = packet->seqNum;
        }

        m_receivedBits[packet->seqNum % 1024] = true;

        ReceiveData data;
        data.data = (char*)buf + sizeof(PacketReliable);
        data.bytes = bytes - sizeof(PacketReliable);
        data.msgType = packet->msgType;
        data.callback = true;
        return data;
    }
}
