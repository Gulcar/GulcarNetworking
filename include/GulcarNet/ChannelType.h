#pragma once

#include <stdint.h>

namespace Net
{
    enum ChannelType : uint8_t
    {
        Unreliable,
        UnreliableDiscardOld,
        Reliable,
    };
}
