#ifndef Bits_hpp
#define Bits_hpp
#pragma once

#include <cstdint>
#include <vector>
#include <limits>
#include <type_traits>

template<size_t NBits>
struct BitMask
{
    static constexpr size_t WordBits = 64;
    static constexpr size_t NWords = (NBits + WordBits - 1) / WordBits;
    uint64_t words[NWords]{};

    void set(size_t index) {
        words[index / WordBits] |= (uint64_t(1) << (index % WordBits));
    }

    void clear(size_t index) {
        words[index / WordBits] &= ~(uint64_t(1) << (index % WordBits));
    }

    bool isSet(size_t index) const {
        return (words[index / WordBits] & (uint64_t(1) << (index % WordBits))) != 0;
    }

    void clearAll() {
        for (size_t i = 0; i < NWords; ++i)
            words[i] = 0;
    }

    void setAll() {
        for (size_t i = 0; i < NWords; ++i)
            words[i] = ~uint64_t(0);
    }
};


template <typename T>
    requires std::is_integral_v<T>
class SizedBitField
{
public:
    SizedBitField()
        : m_field{ 0 } {}
    SizedBitField(T startField)
        : m_field{ startField } {}
    template <typename ENUM>
        requires requires(ENUM e) {
            { static_cast<uint32_t>(e) };
    }
    SizedBitField(ENUM startState) {
        m_field = 0;
        m_field |= (T(1) << ((uint32_t)startState));
    }

    template <typename ENUM>
        requires requires(ENUM e) {
            { static_cast<uint32_t>(e) };
    }
    void setByEnum(ENUM index) {
        m_field |= (T(1) << ((uint32_t)index));
    }
    void set(uint32_t index) {
        m_field |= (T(1) << (index));
    }
    template <typename ENUM>
        requires requires(ENUM e) {
            { static_cast<uint32_t>(e) };
    }
    void clearByEnum(ENUM index) {
        m_field &= ~(T(1) << ((uint32_t)index));
    }
    void clear(uint32_t index) {
        m_field &= ~(T(1) << (index));
    }

    void toggle(uint32_t index) {
        m_field ^= (T(1) << (index));
    }

    template <typename ENUM>
        requires requires(ENUM e) {
            { static_cast<uint32_t>(e) };
    }
    bool hasFlag(ENUM flag) const {
        return (m_field & static_cast<T>(flag)) != 0;
    }

    bool isSet(uint32_t index) const {
        return (m_field & (T(1) << (index))) != 0;
    }

    void setByBool(uint32_t index, bool target) {
        if (target)
            set(index);
        else
            clear(index);
    }

    void clearAll() {
        m_field = 0;
    }

    void setAll() {
        m_field = static_cast<T>(-1);
    }

    void copyField(T fieldValue) {
        m_field = fieldValue;
    }



    T& getField() const { return const_cast<T&>(m_field); }
    T& getField() { return m_field; }

    template <typename U>
    U asType() {
        return static_cast<U>(m_field);
    }

private:
    T m_field;
};


class BitField
{
public:
    BitField(size_t numBits)
        : m_numBits{ numBits },
        m_words{ (numBits + WordBits - 1) / WordBits, 0 } {}

    void set(size_t index) {
        m_words[index / WordBits] |= (uint64_t(1) << (index % WordBits));
    }

    void clear(size_t index) {
        m_words[index / WordBits] &= ~(uint64_t(1) << (index % WordBits));
    }

    void toggle(size_t index) {
        m_words[index / WordBits] ^= (uint64_t(1) << (index % WordBits));
    }

    bool isSet(size_t index) const {
        return (m_words[index / WordBits] & (uint64_t(1) << (index % WordBits))) != 0;
    }

    void setByBool(size_t index, bool target) {
        if (target)
            set(index);
        else
            clear(index);
    }

    void clearAll() {
        for (auto& word : m_words)
            word = 0;
    }

    void setAll() {
        for (auto& word : m_words)
            word = ~uint64_t(0);
    }

    size_t size() const {
        return m_numBits;
    }

private:
    static constexpr size_t WordBits = 64;
    size_t m_numBits;
    std::vector<uint64_t> m_words;
};



class BitFieldView
{
public:
    BitFieldView(uint64_t* data, size_t numBits)
        : m_data(data),
        m_numBits(numBits) {}

    void set(size_t index) {
        m_data[index / WordBits] |= (uint64_t(1) << (index % WordBits));
    }

    void clear(size_t index) {
        m_data[index / WordBits] &= ~(uint64_t(1) << (index % WordBits));
    }

    void toggle(size_t index) {
        m_data[index / WordBits] ^= (uint64_t(1) << (index % WordBits));
    }

    bool isSet(size_t index) const {
        return (m_data[index / WordBits] & (uint64_t(1) << (index % WordBits))) != 0;
    }

    void clearAll() {
        const size_t numWords = (m_numBits + WordBits - 1) / WordBits;
        for (size_t i = 0; i < numWords; ++i)
            m_data[i] = 0;
    }

    void setAll() {
        const size_t numWords = (m_numBits + WordBits - 1) / WordBits;
        for (size_t i = 0; i < numWords; ++i)
            m_data[i] = ~uint64_t(0);
    }

    size_t size() const {
        return m_numBits;
    }

private:
    static constexpr size_t WordBits = 64;
    uint64_t* m_data = nullptr;
    size_t m_numBits = 0;
};


#endif