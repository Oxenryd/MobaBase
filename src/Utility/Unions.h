#ifndef UNIONS_H
#define UNIONS_H



template <size_t U, size_t V, std::integral T>
struct MaskState
{
private:
    union
    {
        T __raw;
        struct
        {
            T m_mask    : U;
            T m_state   : V;
        };
    };
public:
    static_assert(U + V == sizeof(T) * 8);
    MaskState(T mask, T state) :
        m_mask{mask}, m_state{state} {}

    T mask() const { return m_mask; }
    T state() const { return m_state; }
    void setMask(T mask) { m_mask = mask; }
    void setState(T state) { m_state = state; }
};

//union State2Mask6
//{
//    State2Mask6(uint8_t mask, uint8_t state) :
//        state{state}, mask{mask} {}
//    uint8_t raw;
//    struct
//    {
//        uint8_t state : 2;
//        uint8_t mask : 6;
//    };
//};

#endif