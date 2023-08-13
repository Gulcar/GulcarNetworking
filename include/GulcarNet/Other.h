#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <string_view>

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

        template<typename T>
        Buf(const T& data)
            : data(&data), bytes(sizeof(T)) {}

        /** pointer to the refering data */
        const void* data;
        /** size of the data in bytes */
        size_t bytes;
    };
}
