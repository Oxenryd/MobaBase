#ifndef Bits_hpp
#define Bits_hpp
#pragma once

#include <cstdint>
#include <cassert>
#include <vector>
#include <limits>
#include <type_traits>


template<size_t NBits>
struct BitMask
{
    static constexpr size_t WordBits = 64;
    static constexpr size_t NWords = (NBits + WordBits - 1) / WordBits;
    uint64_t words[NWords]{};

    void set(const size_t index) {
        words[index / WordBits] |= static_cast<uint64_t>(1) << (index % WordBits);
    }

    void clear(const size_t index) {
        words[index / WordBits] &= ~(static_cast<uint64_t>(1) << (index % WordBits));
    }

    [[nodiscard]] bool isSet(const size_t index) const {
        return (words[index / WordBits] & static_cast<uint64_t>(1) << (index % WordBits)) != 0;
    }

    void clearAll() {
        for (size_t i = 0; i < NWords; ++i)
            words[i] = 0;
    }

    void setAll() {
        for (size_t i = 0; i < NWords; ++i)
            words[i] = ~static_cast<uint64_t>(0);
    }
};


//template <typename T>
//    requires std::is_integral_v<T>
//class SizedBitField
//{
//public:
//    SizedBitField()
//        : m_field{ 0 } {}
//    SizedBitField(T startField)
//        : m_field{ startField } {}
//    template <typename ENUM>
//        requires requires(ENUM e) {
//            { static_cast<uint32_t>(e) };
//    }
//    SizedBitField(ENUM startState) {
//        m_field = 0;
//        m_field |= (T(1) << ((uint32_t)startState));
//    }
//
//    template <typename ENUM>
//        requires requires(ENUM e) {
//            { static_cast<uint32_t>(e) };
//    }
//    void setByEnum(ENUM index) {
//        m_field |= (T(1) << ((uint32_t)index));
//    }
//    void set(uint32_t index) {
//        m_field |= (T(1) << (index));
//    }
//    template <typename ENUM>
//        requires requires(ENUM e) {
//            { static_cast<uint32_t>(e) };
//    }
//    void clearByEnum(ENUM index) {
//        m_field &= ~(T(1) << ((uint32_t)index));
//    }
//    void clear(uint32_t index) {
//        m_field &= ~(T(1) << (index));
//    }
//
//    void toggle(uint32_t index) {
//        m_field ^= (T(1) << (index));
//    }
//
//    template <typename ENUM>
//        requires requires(ENUM e) {
//            { static_cast<uint32_t>(e) };
//    }
//    bool hasFlag(ENUM flag) const {
//        return (m_field & static_cast<T>(flag)) != 0;
//    }
//
//    bool isSet(uint32_t index) const {
//        return (m_field & (T(1) << (index))) != 0;
//    }
//
//    void setByBool(uint32_t index, bool target) {
//        if (target)
//            set(index);
//        else
//            clear(index);
//    }
//
//    void clearAll() {
//        m_field = 0;
//    }
//
//    void setAll() {
//        m_field = static_cast<T>(-1);
//    }
//
//    void copyField(T fieldValue) {
//        m_field = fieldValue;
//    }
//
//
//
//    T& getField() const { return const_cast<T&>(m_field); }
//    T& getField() { return m_field; }
//
//    template <typename U>
//    U asType() {
//        return static_cast<U>(m_field);
//    }
//
//private:
//    T m_field;
//};

template <typename T>
    requires (std::is_integral_v<T>&& std::is_unsigned_v<T>) // avoid UB on signed shifts
class SizedBitField
{
public:
    // ----- types/limits -----
    static constexpr uint32_t bit_count = std::numeric_limits<T>::digits;

    // ----- ctors -----
    constexpr SizedBitField() noexcept : m_field{ 0 } {}

    constexpr explicit SizedBitField(T startField) noexcept : m_field{ startField } {}

    // Treat ENUM as a *mask* (e.g., enum Flags { A = 1u<<0, B = 1u<<1, ... })
    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    constexpr explicit SizedBitField(ENUM startMask) noexcept
        : m_field{ static_cast<T>(startMask) } {}

    // ----- set/clear by index (bit position) -----
    constexpr void set(const uint32_t index) noexcept {
        assert(index < bit_count);
        m_field |= T(1) << index;
    }

    constexpr void clear(const uint32_t index) noexcept {
        assert(index < bit_count);
        m_field &= ~(T(1) << index);
    }

    constexpr void toggle(const uint32_t index) noexcept {
        assert(index < bit_count);
        m_field ^= T(1) << index;
    }

    // ----- set/clear by enum mask -----
    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    constexpr void setByEnum(ENUM mask) noexcept {
        // FIX: Treat ENUM as a mask, not an index
        m_field |= static_cast<T>(mask);
    }

    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    constexpr void clearByEnum(ENUM mask) noexcept {
        // FIX: Treat ENUM as a mask, not an index
        m_field &= ~static_cast<T>(mask);
    }

    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    constexpr void toggleByEnum(ENUM mask) noexcept {
        m_field ^= static_cast<T>(mask);
    }

    // ----- queries -----
    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    [[nodiscard]] constexpr bool hasFlag(ENUM mask) const noexcept {
        return (m_field & static_cast<T>(mask)) != 0;
    }

    [[nodiscard]] constexpr bool isSet(const uint32_t index) const noexcept {
        assert(index < bit_count);
        return (m_field & T(1) << index) != 0;
    }

    // ----- utilities -----
    constexpr void setByBool(const uint32_t index, const bool target) noexcept {
        target ? set(index) : clear(index);
    }

    template <typename ENUM>
        requires std::is_enum_v<ENUM>
    constexpr void setByBool(ENUM mask, bool target) noexcept {
        target ? setByEnum(mask) : clearByEnum(mask);
    }

    constexpr void clearAll() noexcept { m_field = 0; }

    constexpr void setAll() noexcept { m_field = ~T{ 0 }; }

    constexpr void copyField(T fieldValue) noexcept { m_field = fieldValue; }

    // Accessors (fixed const-correctness)
    [[nodiscard]] constexpr T        value() const noexcept { return m_field; }
    [[nodiscard]] constexpr const T& getField() const noexcept { return m_field; }
    constexpr T& getField() noexcept { return m_field; } // mutable ref

    template <typename U>
    [[nodiscard]] constexpr U asType() const noexcept {
        return static_cast<U>(m_field);
    }

    // Optional: implicit conversion to T for convenience (explicit to avoid surprises)
    explicit constexpr operator T() const noexcept { return m_field; }

private:
    T m_field;
};




class BitField
{
public:
    explicit BitField(const size_t numBits)
        : m_numBits{ numBits },
        m_words{ (numBits + WordBits - 1) / WordBits, 0 } {}

    void set(const size_t index) {
        m_words[index / WordBits] |= static_cast<uint64_t>(1) << (index % WordBits);
    }

    void clear(const size_t index) {
        m_words[index / WordBits] &= ~(static_cast<uint64_t>(1) << (index % WordBits));
    }

    void toggle(const size_t index) {
        m_words[index / WordBits] ^= static_cast<uint64_t>(1) << (index % WordBits);
    }

    [[nodiscard]] bool isSet(const size_t index) const {
        return (m_words[index / WordBits] & static_cast<uint64_t>(1) << (index % WordBits)) != 0;
    }

    void setByBool(const size_t index, const bool target) {
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
            word = ~static_cast<uint64_t>(0);
    }

    [[nodiscard]] size_t size() const {
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
    BitFieldView(uint64_t* data, const size_t numBits)
        : m_data(data),
        m_numBits(numBits) {}

    void set(const size_t index) {
        m_data[index / WordBits] |= static_cast<uint64_t>(1) << (index % WordBits);
    }

    void clear(const size_t index) {
        m_data[index / WordBits] &= ~(static_cast<uint64_t>(1) << (index % WordBits));
    }

    void toggle(const size_t index) {
        m_data[index / WordBits] ^= (static_cast<uint64_t>(1) << (index % WordBits));
    }

    [[nodiscard]] bool isSet(const size_t index) const {
        return (m_data[index / WordBits] & (static_cast<uint64_t>(1) << (index % WordBits))) != 0;
    }

    void clearAll() {
        const size_t numWords = (m_numBits + WordBits - 1) / WordBits;
        for (size_t i = 0; i < numWords; ++i)
            m_data[i] = 0;
    }

    void setAll() {
        const size_t numWords = (m_numBits + WordBits - 1) / WordBits;
        for (size_t i = 0; i < numWords; ++i)
            m_data[i] = ~static_cast<uint64_t>(0);
    }

    [[nodiscard]] size_t size() const {
        return m_numBits;
    }

private:
    static constexpr size_t WordBits = 64;
    uint64_t* m_data = nullptr;
    size_t m_numBits = 0;
};


#endif