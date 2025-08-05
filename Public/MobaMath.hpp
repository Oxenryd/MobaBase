#ifndef MOBAMATH_HPP
#define MOBAMATH_HPP

#include <concepts>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "GlobalMacros.h"

namespace MMath
{
	INLINE static glm::mat4x4 composeTRS(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {

        glm::mat4 rot = glm::mat4_cast(rotation);
        rot[0] *= scale.x;
        rot[1] *= scale.y;
        rot[2] *= scale.z;

        rot[3] = glm::vec4(position, 1.0f);
        return rot;
	}

    INLINE static glm::mat4x4 composeTRS_Inverse(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {

        glm::vec3 invScale = 1.0f / scale;
        glm::quat invRotation = glm::conjugate(rotation);
        glm::vec3 invTranslation = -(invRotation * (position * invScale));

        return composeTRS(invTranslation, invRotation, invScale);
    }
}


template<int N> struct RectTraits;

template<>
struct RectTraits<8>
{
    using SignedVec2 = glm::i8vec2;
    using UnsignedVec2 = glm::u8vec2;
};

template<>
struct RectTraits<16>
{
    using SignedVec2 = glm::i16vec2;
    using UnsignedVec2 = glm::u16vec2;
};

template<>
struct RectTraits<32>
{
    using SignedVec2 = glm::i32vec2;
    using UnsignedVec2 = glm::u32vec2;
};

template<>
struct RectTraits<64>
{
    using SignedVec2 = glm::i64vec2;
    using UnsignedVec2 = glm::u64vec2;
};



template<int N>
struct IntRect
{
    using Traits = RectTraits<N>;

    IntRect(int64_t x, int64_t y, int64_t width, int64_t height) :
        position{x, y}, size{width, height} {}

//#ifdef BUILD_WIN
//
//    IntRect(const RECT& winRect) {
//        position = { winRect.left, winRect.top };
//        size = { winRect.right - winRect.left, winRect.bottom - winRect.top };
//    }
//#endif

    typename Traits::SignedVec2 position;
    typename Traits::UnsignedVec2 size;


};

struct uint24_t
{
    uint8_t data[3]{};

    // Implicit conversion to uint32_t
    operator uint32_t() const {
        return (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[0]));
    }

    // Assignment from uint32_t
    uint24_t& operator=(uint32_t value) {
        data[0] = static_cast<uint8_t>(value & 0xFF);
        data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        return *this;
    }

    // Constructors
    uint24_t() = default;
    uint24_t(uint32_t value) { *this = value; }

    // Comparison operators
    bool operator==(const uint24_t& other) const { return uint32_t(*this) == uint32_t(other); }
    bool operator!=(const uint24_t& other) const { return !(*this == other); }


    // Arithmetic operators (operate via conversion)
    uint24_t operator+(uint32_t rhs) const { return uint32_t(*this) + rhs; }
    uint24_t operator-(uint32_t rhs) const { return uint32_t(*this) - rhs; }
    uint24_t operator*(uint32_t rhs) const { return uint32_t(*this) * rhs; }
    uint24_t operator/(uint32_t rhs) const { return uint32_t(*this) / rhs; }
    uint24_t operator%(uint32_t rhs) const { return uint32_t(*this) % rhs; }

    uint24_t& operator+=(uint32_t rhs) { return *this = *this + rhs; }
    uint24_t& operator-=(uint32_t rhs) { return *this = *this - rhs; }
    uint24_t& operator*=(uint32_t rhs) { return *this = *this * rhs; }
    uint24_t& operator/=(uint32_t rhs) { return *this = *this / rhs; }
    uint24_t& operator%=(uint32_t rhs) { return *this = *this % rhs; }

    // Bitwise operators
    uint24_t operator&(uint32_t rhs) const { return uint32_t(*this) & rhs; }
    uint24_t operator|(uint32_t rhs) const { return uint32_t(*this) | rhs; }
    uint24_t operator^(uint32_t rhs) const { return uint32_t(*this) ^ rhs; }
    uint24_t operator~() const { return ~uint32_t(*this); }

    uint24_t& operator&=(uint32_t rhs) { return *this = *this & rhs; }
    uint24_t& operator|=(uint32_t rhs) { return *this = *this | rhs; }
    uint24_t& operator^=(uint32_t rhs) { return *this = *this ^ rhs; }

    // Increment/decrement
    uint24_t& operator++() { return *this += 1; }
    uint24_t operator++(int) { uint24_t tmp = *this; ++(*this); return tmp; }
    uint24_t& operator--() { return *this -= 1; }
    uint24_t operator--(int) { uint24_t tmp = *this; --(*this); return tmp; }
};

#endif