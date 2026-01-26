//
// Created by oxenryd on 2026-01-18.
//

#ifndef MOBABASE_FORMAT_FIXES_HPP
#define MOBABASE_FORMAT_FIXES_HPP

#include <format>
#include <cstdint>

template<>
struct std::formatter<uint16_t> : std::formatter<uint32_t> {
    auto format(uint16_t value, auto& ctx) const {
        return std::formatter<uint32_t>::format(static_cast<uint32_t>(value), ctx);
    }
};

// Also add for uint8_t if you use it:
template<>
struct std::formatter<uint8_t> : std::formatter<uint32_t> {
    auto format(uint8_t value, auto& ctx) const {
        return std::formatter<uint32_t>::format(static_cast<uint32_t>(value), ctx);
    }
};

// And signed versions if needed:
template<>
struct std::formatter<int16_t> : std::formatter<int32_t> {
    auto format(int16_t value, auto& ctx) const {
        return std::formatter<int32_t>::format(static_cast<int32_t>(value), ctx);
    }
};

template<>
struct std::formatter<int8_t> : std::formatter<int32_t> {
    auto format(int8_t value, auto& ctx) const {
        return std::formatter<int32_t>::format(static_cast<int32_t>(value), ctx);
    }
};

#endif //MOBABASE_FORMAT_FIXES_HPP