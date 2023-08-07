#pragma once

#include <stdint.h>

namespace Net
{
    enum SendType : uint8_t
    {
        Unreliable, // just packets (packet loss, out of order)
        UnreliableDiscardOld, // will discard the packet if a newer packet with same msgType already arrived (still packet loss)
        Reliable, // will resend if no ack (ordered)
    };
}
