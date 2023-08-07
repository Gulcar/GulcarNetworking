#pragma once

#include <stdint.h>
#include <string>
#include <cstring>

namespace Net
{
    enum SendType : uint8_t
    {
        Unreliable, // just packets (packet loss, out of order)
        UnreliableDiscardOld, // will discard the packet if a newer packet with same msgType already arrived (still packet loss)
        Reliable, // will resend if no ack (ordered)
    };

    // const buffer
    struct Buf
    {
        Buf(const void* data, size_t bytes)
            : data(data), bytes(bytes) {}
        Buf(const char* data)
            : data(data), bytes(strlen(data)) {}
        Buf(const std::string& data)
            : data(data.data()), bytes(data.length()) {}

        template<typename T>
        Buf(const T& data)
            : data(&data), bytes(sizeof(T)) {}

        const void* data;
        size_t bytes;
    };
}
