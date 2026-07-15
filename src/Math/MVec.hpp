#ifndef _MVEC_HPP_
#define _MVEC_HPP_

#include "Concepts.h"
#include "Globals.hpp"

#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <xmmintrin.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

using namespace Consts;

namespace MMath
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // FORWARDS
    template<FloatingPointConcept ValT, size_t N>
        requires (N >= MIN_VEC_SIZE && N <= MAX_VEC_SIZE)
    struct VecMembers {};

    template<FloatingPointConcept ValT, size_t N, typename ...Args>
        requires (N >= MIN_VEC_SIZE && N <= MAX_VEC_SIZE)
    static VecMembers<ValT, N> constructVecMembers(Args&&... args);

    template<FloatingPointConcept ValT, size_t N>
        requires (N >= MIN_VEC_SIZE && N <= MAX_VEC_SIZE)
    static VecMembers<ValT, N> constructVecMembers(const VecMembers<ValT, N>& other);
    
    template<FloatingPointConcept ValT, size_t N>
        requires (N >= MIN_VEC_SIZE  && N <= MAX_VEC_SIZE)       
    struct MVec;

    template<FloatingPointConcept ValT, size_t N, FloatingPointConcept OtherValT, size_t OtherN>
    inline bool almostEquals(
        const MVec<ValT, N>& first,
        const MVec<OtherValT, OtherN>& second,
        double epsilonFactor = 1.0 );


    // VEC MEMBER UNION SPECIALIZATIONS /////////////////////////////////////////////////////////////////
    template<FloatingPointConcept ValT>
    struct alignas(sizeof(ValT) * 2) VecMembers<ValT, 2>
    {
        static constexpr size_t size = 2;
        static constexpr size_t buffer_size = 2;
        union {
            mutable ValT raw[buffer_size];
            struct { ValT x, y; };
            struct { ValT r, g; };
        };
        constexpr VecMembers<ValT, size>() {
            std::fill(raw, raw + size, static_cast<ValT>(0.0));
        }
        VecMembers<ValT, size>(const VecMembers<ValT, size>& other) {
            x = other.x;
            y = other.y;
        }
        VecMembers<ValT, size>& operator=(const VecMembers<ValT, size>& rhs) {
            x = rhs.x;
            y = rhs.y;
            return *this;
        }
    };

    template<FloatingPointConcept ValT>
    struct alignas(sizeof(ValT) * 4) VecMembers<ValT, 3>
    {
    protected:
            inline auto getAsSimdReg() const {
            if constexpr (sizeof(ValT) == 8) {
                return _mm256_load_pd(reinterpret_cast<double*>(&raw));
            } else {
                return _mm_load_ps(reinterpret_cast<float*>(&raw));
            }
        }
    public:
        static constexpr size_t size = 3;
        static constexpr size_t buffer_size = 4;
        union {
            mutable ValT raw[buffer_size];
            struct { ValT x, y, z; };
            struct { ValT r, g, b; };
        };
        constexpr VecMembers<ValT, size>() {
            std::fill(raw, raw + size, 0.0);
        }
        VecMembers<ValT, size>(const VecMembers<ValT, size>& other) {
            if constexpr (sizeof(ValT) == 8)
                _mm256_store_pd(reinterpret_cast<double*>(&raw), other.getAsSimdReg());
            else 
                _mm_store_ps(reinterpret_cast<float*>(&raw), other.getAsSimdReg());
        }
        VecMembers<ValT, size>& operator=(const VecMembers<ValT, size>& rhs) {
            if constexpr (sizeof(ValT) == 8)
                _mm256_store_pd(reinterpret_cast<double*>(&raw), rhs.getAsSimdReg());
            else 
                _mm_store_ps(reinterpret_cast<float*>(&raw), rhs.getAsSimdReg());
            return *this;
        }
    };

    template<FloatingPointConcept ValT>
    struct alignas(sizeof(ValT) * 4) VecMembers<ValT, 4>
    {
    protected:
            inline auto getAsSimdReg() const {
            if constexpr (sizeof(ValT) == 8) {
                return _mm256_load_pd(reinterpret_cast<double*>(&raw));
            } else {
                return _mm_load_ps(reinterpret_cast<float*>(&raw));
            }
        }
    public:
        static constexpr size_t size = 4;
        static constexpr size_t buffer_size = 4;
        union {
            mutable ValT raw[buffer_size];
            struct { ValT x, y, z, w; };
            struct { ValT r, g, b, a; };
        };
        constexpr VecMembers<size, ValT>() {
            std::fill(raw, raw + size, 0.0);
        }
        VecMembers<ValT, size>(const VecMembers<ValT, size>& other) {
            if constexpr (sizeof(ValT) == 8)
                _mm256_store_pd(reinterpret_cast<double*>(&raw), other.getAsSimdReg());
            else 
                _mm_store_ps(reinterpret_cast<float*>(&raw), other.getAsSimdReg());
        }
        VecMembers<ValT, size>& operator=(const VecMembers<ValT, size>& rhs) {
            if constexpr (sizeof(ValT) == 8)
                _mm256_store_pd(reinterpret_cast<double*>(&raw), rhs.getAsSimdReg());
            else 
                _mm_store_ps(reinterpret_cast<float*>(&raw), rhs.getAsSimdReg());
            return *this;
        }
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////



    // VECTOR BASE //////////////////////////////////////////////////////////////////////////////////////
    template<FloatingPointConcept ValT, size_t N>
        requires (N >= MIN_VEC_SIZE  && N <= MAX_VEC_SIZE)       
    struct MVec : public VecMembers<ValT, N>
    {
        friend struct MVec;
    public:
        static constexpr bool simd_aligned() { return N > 2; }
        ~MVec() {}
        constexpr MVec() {}
       
        template<typename SimdReg>
            requires (
                (N > 2) 
                && (std::is_same_v<SimdReg, __m128> || std::is_same_v<SimdReg, __m256> )
            )
        MVec(const SimdReg reg) {
            if constexpr (sizeof(ValT) == 4) {
                static_assert(sizeof(SimdReg) == 4, "Wrong SIMD register type for MVec<N, float>");
                _mm_store_ps(data(), reg);
            } else {
                static_assert(sizeof(SimdReg) == 8, "Wrong SIMD register type for MVec<N, double>");
                _mm256_store_pd(data(), reg);
            }
        }
        template<typename SimdReg>
            requires (
                (N > 2) 
                && (std::is_same_v<SimdReg, __m128> || std::is_same_v<SimdReg, __m256> )
            )
        MVec& operator=(const SimdReg reg) {
            if constexpr (sizeof(ValT) == 4) {
                static_assert(sizeof(SimdReg) == 4, "Wrong SIMD register type for MVec<N, ValT>");
                _mm_store_ps(data(), reg);
            } else {
                static_assert(sizeof(SimdReg) == 8, "Wrong SIMD register type for MVec<N, ValT>");
                _mm256_store_pd(data(), reg);
            }
            return *this;
        }

        MVec(const MVec& other)
            : VecMembers<N, ValT>{other}
        {}

        MVec& operator=(const MVec& rhs) {
            if (this == &rhs)
                return *this;

            this->raw = rhs.raw;
            return *this;
        }

        template<size_t OtherN>
            requires (OtherN != N)
        MVec(const MVec<OtherN, ValT>& other) {
            if constexpr (simd_aligned() && other.simd_aligned()) {
                *this = other.getAsSimdReg();
            } else {
                constexpr size_t count = std::min(N, OtherN);         
                for (size_t i = 0; i < count; ++i)
                    this->raw[i] = other[i];

                if constexpr (N > OtherN) {
                    for (size_t i = count; i < N; ++i)
                        this->raw[i] = static_cast<ValT>(0.0);
                }
            }
        }

        template<size_t OtherN>
            requires (OtherN != N)
        MVec& operator=(const MVec<ValT, OtherN>& rhs) {
            constexpr size_t count = std::min(N, OtherN);         
            for (size_t i = 0; i < count; ++i)
                this->raw[i] = rhs.raw[i];

            if constexpr (N > OtherN) {
                for (size_t i = count; i < N; ++i)
                    this->raw[i] = static_cast<ValT>(0.0);
            }           
            return *this;
        }

        template<typename ...Args>
            requires (sizeof...(Args) == N)
        MVec(Args&&... args) 
            : VecMembers<ValT, N>{ constructVecMembers<ValT, N>(args...)}
        {}

        const ValT&    operator[](const size_t idx) const { return this->raw[idx]; }
        ValT&          operator[](const size_t idx) { return this->raw[idx]; }
        MVec operator-() const {
            MVec result;
            for (size_t i = 0; i < N; ++i)
                result[i] = -(*this[i]);
            return result;
        }
        ValT* data() { return this->raw; }

        template<FloatingPointConcept OtherValT, size_t OtherN>
        bool operator==(const MVec<OtherValT, OtherN>& other) const {
#           ifdef MMATH_PREFER_ALMOST_EQUALS_FP_EQUALITY
                return almostEquals(*this, other);
#           else
                if constexpr (N != OtherN)
                    return false;
                else if constexpr (!std::is_same_v<ValT, OtherValT>)
                    return false;
                else {
                    if (this == &other)
                        return true;

                    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                        if (this->raw[i] != other.raw[i])
                            return false;
                    }

                    return true;
                }    
#           endif
        }
    };

    using Vec2  = MVec<float, 2>;
    using Vec3  = MVec<float, 3>;
    using Vec4  = MVec<float, 4>;
    using Vec2d = MVec<double, 2>;
    using Vec3d = MVec<double, 3>;
    using Vec4d = MVec<double, 4>;

    /////////////////////////////////////////////////////////////////////////////////////////////////////


    // FREE FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////

    inline bool almostEqual(__m128 a, __m128 b, float epsilon) {
        __m128 diff = _mm_sub_ps(a, b);
        __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        __m128 absDiff = _mm_and_ps(diff, absMask);
        __m128 eps = _mm_set1_ps(epsilon);
        __m128 cmp = _mm_cmple_ps(absDiff, eps);
        return _mm_movemask_ps(cmp) == 0b1111;
    }
    inline bool almostEqual(__m256d a, __m256d b, double epsilon) {
        __m256d diff = _mm256_sub_pd(a, b);
        __m256d absMask = _mm256_castsi256_pd(_mm256_set1_epi64x(0x7FFFFFFFFFFFFFFFLL));
        __m256d absDiff = _mm256_and_pd(diff, absMask);
        __m256d cmp = _mm256_cmp_pd(absDiff, _mm256_set1_pd(epsilon), _CMP_LE_OQ);
        return _mm256_movemask_pd(cmp) == 0b1111;
    }

        // Operators on Vec
    template<size_t N, FloatingPointConcept ValT, size_t OtherN, FloatingPointConcept OtherValT>
    inline bool almostEquals(
        const MVec<ValT, N>& first,
        const MVec<OtherValT, OtherN>& second,
        double epsilonFactor)
    {
        if constexpr (N != OtherN)
            return false;
        else {
            if constexpr (std::is_same_v<ValT, OtherValT>) {
                return almostEqual(first.raw.getAsSimdReg(), second.raw.getAsSimdReg());
            } else {
                constexpr size_t minSize = (sizeof(ValT) == 4 || sizeof(OtherValT) == 4)
                    ? 4
                    : 8;

                if constexpr (minSize == 4) {
                    for (size_t i = 0; i < N; ++i) {
                        if (std::abs(
                                static_cast<float>(second.raw[i]) - static_cast<float>(second.raw[i]))
                                    > static_cast<float>(epsilonFactor) * EPSILON_f)
                            return false;
                    }
                } else {
                    for (size_t i = 0; i < N; ++i) {
                        if (std::abs(
                                static_cast<double>(second.raw[i]) - static_cast<double>(second.raw[i]))
                                    > static_cast<double>(epsilonFactor) * EPSILON_d)
                            return false;
                    }
                }

            }
            if (&first == &second)
                return true;
            return true;
        }     
    }


    template<size_t N, FloatingPointConcept ValT, size_t OtherN, FloatingPointConcept OtherValT>
    inline auto operator+(const MVec<ValT, N>& a, const MVec<OtherValT, OtherN>& b) {
        constexpr size_t smallest_n = std::min(N, OtherN);
        constexpr auto chosen_precision = []() {
            if constexpr (sizeof(ValT) == 8 && sizeof(OtherValT) == 8)
                return type_tag<double>{};
            else
                return type_tag<float>{};
        }();
        using Precision_t = typename decltype(chosen_precision)::type;

        MVec<Precision_t, smallest_n> result;
        if constexpr (smallest_n < 3) {
            result.x = a.x + b.x;
            result.y = a.y + b.y;
        } else if constexpr (std::is_same_v<ValT, OtherValT>) {
            if constexpr (std::is_same_v<Precision_t, double>) {
                _mm256_store_pd(result.data(), _mm256_add_pd(a.raw.getAsSimdReg(), b.raw.getAsSimdReg()));
            } else {
                _mm_store_ps(result.data(), _mm_add_ps(a.raw.getAsSimdReg(), b.raw.getAsSimdReg()));
            }
        }
        return result;
    }
    template<size_t N, FloatingPointConcept ValT, size_t OtherN, FloatingPointConcept OtherValT>
    inline MVec<ValT, N> operator-(const MVec<ValT, N>& a, const MVec<ValT, N>& b) {
        MVec<N, ValT> result;
        if constexpr (N > OtherN) {
            result = {};
            for (size_t i = 0; i < OtherN; ++i)
                result[i] = a[i] - b[i];
        } else {
            for (size_t i = 0; i < N; ++i)
                result[i] = a[i] - b[i];
        }
        return result;
    }

    template<size_t N, FloatingPointConcept ValT, FloatingPointConcept Scalar>
    inline MVec<ValT, N> operator*(const MVec<ValT, N>& lhs, Scalar rhs) {
        MVec<ValT, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = lhs[i] * static_cast<ValT>(rhs);
        return result;
    }
    template<size_t N, FloatingPointConcept ValT, FloatingPointConcept Scalar>
    inline MVec<ValT, N> operator*(Scalar lhs, const MVec<ValT, N>& rhs) {
        MVec<ValT, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = rhs[i] * static_cast<ValT>(lhs);
        return result;
    }
    template<size_t N, FloatingPointConcept ValT, FloatingPointConcept Scalar>
    inline MVec<ValT, N> operator/(const MVec<ValT, N>& lhs, Scalar rhs) {
        MVec<ValT, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = lhs[i] / static_cast<ValT>(rhs);
        return result;
    }
    template<size_t N, FloatingPointConcept ValT, FloatingPointConcept Scalar>
    inline MVec<ValT, N> operator/(Scalar lhs, const MVec<ValT, N>& rhs) {
        MVec<ValT, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = rhs[i] / static_cast<ValT>(lhs);
        return result;
    }

    template<size_t N, FloatingPointConcept ValT>
        requires (N > MIN_VEC_SIZE && N <= MAX_VEC_SIZE)
    inline VecMembers<ValT, N> constructVecMembers(const VecMembers<ValT, N>& other) {
        VecMembers<N, ValT> members{};
        for (size_t i = 0; i < N; ++i)
            members.raw[i] = other.raw[i];
        return members;
    };

    template<size_t N, FloatingPointConcept ValT, typename ...Args>
        requires (N > MIN_VEC_SIZE && N <= MAX_VEC_SIZE)
    inline VecMembers<ValT, N> constructVecMembers(Args&&... args) {
        VecMembers<ValT, N> members{};
        [&]<size_t... I>(std::index_sequence<I...>) {
            ((members.raw[I] = std::forward<Args>(args)), ...);
        }(std::index_sequence_for<Args...>{});
        return members;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////


    // TESTS ////////////////////////////////////////////////////////////////////////////////////////////

    inline static void test() {
		Vec2 vec2{ 1, 2 };
        Vec3 vec3{3, 5, 1};
        MVec<double, 4> test;
        MVec<float, 4> vec4{};
        vec4 = vec3;
        
        auto testVec = vec2 + vec3;

        if (vec2 == vec4) {

        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif