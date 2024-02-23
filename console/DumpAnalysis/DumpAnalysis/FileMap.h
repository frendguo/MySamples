#pragma once
#include <stdint.h>

namespace Page {

    //
    // Page size.
    //

    constexpr uint64_t Size = 0x1000;

    //
    // Page align an address.
    //

    constexpr uint64_t Align(const uint64_t Address) { return Address & ~0xfff; }

    //
    // Extract the page offset off an address.
    //

    constexpr uint64_t Offset(const uint64_t Address) { return Address & 0xfff; }
} // namespace Page