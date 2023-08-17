#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <string_view>
#include <vector>
#include <type_traits>
#include <chrono>

/**
* \file
*/

namespace Net
{
    /** describes the send reliability */
    enum SendType : uint8_t
    {
        /** just packets (packet loss, out of order) */
        Unreliable,
        /** will discard the packet if a newer packet with same msgType already arrived (still packet loss) */
        UnreliableDiscardOld,
        /** will resend if no ack (ordered) */
        Reliable,
    };

    /** const buffer (only refers to data) */
    struct Buf
    {
        Buf(const void* data, size_t bytes)
            : data(data), bytes(bytes) {}
        Buf(const char* data)
            : data(data), bytes(strlen(data)) {}
        Buf(std::string_view data)
            : data(data.data()), bytes(data.length()) {}
        Buf(const std::string& data)
            : data(data.data()), bytes(data.length()) {}

        template<typename T>
        Buf(const std::vector<T>& data)
            : data(data.data()), bytes(data.size() * sizeof(T)) {}

        template<typename T>
        Buf(const T& data)
            : data(&data), bytes(sizeof(T))
        {
            static_assert(std::is_trivial<T>::value, "Net::Buf should only be used with plain old data types!");
        }

        /** pointer to the refering data */
        const void* data;
        /** size of the data in bytes */
        size_t bytes;
    };

    /** Handles the networking statistics. Use Client::GetStats() and Server::GetStats() to retrieve */
    class Statistics
    {
    public:
        /** returns sent B/s (divide by 1000.0f to get kB/s) */
        size_t SentBytesPerSecond() const { return m_avgBytesSent; }
        /** returns sent packets/s */
        int SentPacketsPerSecond() const { return m_avgPacketsSent; }

        /** returns received B/s (divide by 1000.0f to get kB/s) */
        size_t ReceivedBytesPerSecond() const { return m_avgBytesReceived; }
        /** returns received packets/s */
        int ReceivedPacketsPerSecond() const { return m_avgPacketsReceived; }

    private:
        friend class Server;
        friend class Client;

        Statistics() {}

        using Clock = std::chrono::steady_clock;
        Clock::time_point m_time = Clock::now();

        size_t m_bytesSent = 0;
        int m_packetsSent = 0;
        float m_avgBytesSent = 0.0f;
        float m_avgPacketsSent = 0.0f;

        size_t m_bytesReceived = 0;
        int m_packetsReceived = 0;
        float m_avgBytesReceived = 0.0f;
        float m_avgPacketsReceived = 0.0f;

        // 20 ipv4 + 8 udp + (...)
        static constexpr size_t packetHeaderSize = 28;

        void AddPacketSent(size_t bytes)
        {
            m_bytesSent += bytes + packetHeaderSize;
            m_packetsSent++;
        }

        void AddPacketReceived(size_t bytes)
        {
            m_bytesReceived += bytes + packetHeaderSize;
            m_packetsReceived++;
        }

        void UpdateTime()
        {
            if (Clock::now() - m_time > std::chrono::seconds(1))
            {
                constexpr float t = 0.3f;

                m_avgBytesSent = ((1.0f - t) * m_avgBytesSent) + (t * m_bytesSent);
                m_avgPacketsSent = ((1.0f - t) * m_avgPacketsSent) + (t * m_packetsSent);

                m_avgBytesReceived = ((1.0f - t) * m_avgBytesReceived) + (t * m_bytesReceived);
                m_avgPacketsReceived = ((1.0f - t) * m_avgPacketsReceived) + (t * m_packetsReceived);

                m_time += std::chrono::seconds(1);

                m_bytesSent = 0;
                m_packetsSent = 0;
                m_bytesReceived = 0;
                m_packetsReceived = 0;
            }
        }
    };
}
