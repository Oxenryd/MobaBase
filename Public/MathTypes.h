#ifndef MATHTYPES_H
#define MATHTYPES_H

#include <concepts>
#include <glm/glm.hpp>

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

#ifdef BUILD_WIN
    IntRect(const RECT& winRect) {
        position = {winRect.left, winRect.top};
        size = {winRect.right - winRect.left, winRect.bottom - winRect.top};
    }
#endif

    typename Traits::SignedVec2 position;
    typename Traits::UnsignedVec2 size;


};

#endif