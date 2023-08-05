#pragma once

#include <GulcarNet/ChannelType.h>
#include <stdint.h>
#include <bitset>
#include <queue>
#include <chrono>

namespace Net
{
    using Clock = std::chrono::steady_clock;

    struct PacketUnreliable
    {
        uint16_t channelId;
        uint16_t type;

        // data ...
    };

    struct PacketUnreliableDiscardOld
    {
        uint16_t channelId;
        uint16_t type;

        uint16_t seqNum;

        // data ...
    };

    struct PacketReliable
    {
        uint16_t channelId;
        uint16_t type;

        uint16_t seqNum;
        uint16_t ackNum;
        uint32_t ackBits;

        // data ...
    };

    struct WaitingForAck
    {
        Clock::time_point timeSent;
        PacketReliable* packet;
    };

    struct Channel
    {
        ChannelType chType;
        uint16_t localSeqNum = 0;
        uint16_t remoteSeqNum = 0;
        std::bitset<1024> receivedBits;
        std::queue<WaitingForAck> waitingForAck;

        Channel()
        {
            chType = ChannelType::Unreliable;
        }

        Channel(ChannelType type)
        {
            chType = type;
        }
    };
}
