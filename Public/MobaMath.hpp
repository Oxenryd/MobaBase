#ifndef MOBAMATH_HPP
#define MOBAMATH_HPP

#include <immintrin.h>
#include <concepts>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <span>
#include "HlslTypes.h"
#include "GlobalMacros.h"
#include "Frustum.hpp"
//#include "BasicTypes.hpp"



namespace MMath
{
    constexpr const float fPI = 3.14159265359f;
    constexpr const float fTAU = 2 * 3.14159265359f;
    constexpr const double dPI = 3.14159265359;
    constexpr const double dTAU = 2 * 3.14159265359;

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


    INLINE static glm::vec3 getAvgCenter(std::span<BaseVSIn> vertices) {
        glm::vec3 total{ 0 };
        if (vertices.empty())
            return total;

        for (auto& vert : vertices)
            total += vert.pos;

        return total / static_cast<float>(vertices.size());
    }


    inline float hmin4(__m128 v) {
        __m128 t = _mm_min_ps(v, _mm_movehl_ps(v, v));        // [min(2,0), min(3,1), *, *]
        t = _mm_min_ps(t, _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 1, 1, 1)));
        return _mm_cvtss_f32(t);
    }
    inline float hmax4(__m128 v) {
        __m128 t = _mm_max_ps(v, _mm_movehl_ps(v, v));
        t = _mm_max_ps(t, _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 1, 1, 1)));
        return _mm_cvtss_f32(t);
    }
    inline void hminmax8(__m256 v, float& mn, float& mx) {
        __m128 lo = _mm256_castps256_ps128(v);
        __m128 hi = _mm256_extractf128_ps(v, 1);
        mn = hmin4(_mm_min_ps(lo, hi));
        mx = hmax4(_mm_max_ps(lo, hi));
    }
    inline void hmin8(__m256 v, float& mn) {
        __m128 lo = _mm256_castps256_ps128(v);
        __m128 hi = _mm256_extractf128_ps(v, 1);
        mn = hmin4(_mm_min_ps(lo, hi));
    }
    inline void hmax8(__m256 v, float& mx) {
        __m128 lo = _mm256_castps256_ps128(v);
        __m128 hi = _mm256_extractf128_ps(v, 1);
        mx = hmax4(_mm_max_ps(lo, hi));
    }

    INLINE void matrix_multiply_avx2(const float* a, const float* b, float* result) {
        // Load matrix A columns
        __m256 a_col0 = _mm256_load_ps(a);      // a[0-7]
        __m256 a_col1 = _mm256_load_ps(a + 8);  // a[8-15]

        // Load matrix B columns
        __m256 b_col0 = _mm256_load_ps(b);
        __m256 b_col1 = _mm256_load_ps(b + 8);

        // First 8 elements of result
        __m256 r0 = _mm256_mul_ps(a_col0, _mm256_broadcast_ps((const __m128*)(b)));
        r0 = _mm256_fmadd_ps(a_col1, _mm256_broadcast_ps((const __m128*)(b + 4)), r0);
        _mm256_store_ps(result, r0);

        __m256 r1 = _mm256_mul_ps(a_col0, _mm256_broadcast_ps((const __m128*)(b + 8)));
        r1 = _mm256_fmadd_ps(a_col1, _mm256_broadcast_ps((const __m128*)(b + 12)), r1);
        _mm256_store_ps(result + 8, r1);
    }

    
    INLINE glm::mat4 fast_matrix_multiply(const glm::mat4& parent, const glm::mat4& local) {
        alignas(32) glm::mat4 result;
        matrix_multiply_avx2(&parent[0][0], &local[0][0], &result[0][0]);
        return result;
    }

    INLINE Frustum getFrustum(const glm::mat4x4& viewProjection, bool normalize = true) {

        Frustum f;
        const auto m = viewProjection;

        __m128 col0 = _mm_loadu_ps(&m[0][0]);
        __m128 col1 = _mm_loadu_ps(&m[1][0]);
        __m128 col2 = _mm_loadu_ps(&m[2][0]);
        __m128 col3 = _mm_loadu_ps(&m[3][0]);

        {
            __m128 _Tmp3, _Tmp2, _Tmp1, _Tmp0;
            _Tmp0 = _mm_shuffle_ps((col0), (col1), 0x44);
            _Tmp2 = _mm_shuffle_ps((col0), (col1), 0xEE);
            _Tmp1 = _mm_shuffle_ps((col2), (col3), 0x44);
            _Tmp3 = _mm_shuffle_ps((col2), (col3), 0xEE);
            (col0) = _mm_shuffle_ps(_Tmp0, _Tmp1, 0x88);
            (col1) = _mm_shuffle_ps(_Tmp0, _Tmp1, 0xDD);
            (col2) = _mm_shuffle_ps(_Tmp2, _Tmp3, 0x88);
            (col3) = _mm_shuffle_ps(_Tmp2, _Tmp3, 0xDD);
        };

        __m128 _left = _mm_add_ps(col3, col0);
        __m128 _right = _mm_sub_ps(col3, col0);
        __m128 _bottom = _mm_add_ps(col3, col1);
        __m128 _top = _mm_sub_ps(col3, col1);
        __m128 _near = _mm_add_ps(col3, col2);
        __m128 _far = _mm_sub_ps(col3, col2);

        _mm_storeu_ps(f.planes[0].raw, _left);
        _mm_storeu_ps(f.planes[1].raw, _right);
        _mm_storeu_ps(f.planes[2].raw, _bottom);
        _mm_storeu_ps(f.planes[3].raw, _top);
        _mm_storeu_ps(f.planes[4].raw, _near);
        _mm_storeu_ps(f.planes[5].raw, _far);

        //printf("\n\nSIMD:\n");

        //for (int i = 0; i < 6; ++i) {
        //    printf("plane[%d]: %f %f %f %f\n", i, f.planes[i].raw[0], f.planes[i].raw[1], f.planes[i].raw[2], f.planes[i].raw[3]);
        //}

        if (normalize)
            for (size_t i = 0; i < 6; ++i) {

                float* plane = f.planes[i].raw; // contiguous!
                __m128 v = _mm_loadu_ps(plane);
                __m128 lenSq = _mm_dp_ps(v, v, 0x71); // result is [len2, 0, 0, 0]
                __m128 len = _mm_sqrt_ps(lenSq);
                // Broadcast length to all lanes
                len = _mm_shuffle_ps(len, len, 0x00);
                v = _mm_div_ps(v, len);
                _mm_storeu_ps(plane, v);
            }

        return f;
    }

    INLINE static bool sphereVisible(const glm::vec3& center, const float radius, Frustum& frustum) {
        for (int i = 0; i < 6; ++i) {
            const auto& plane = frustum.planes[i];
            float dist = glm::dot(plane.normal, center) + plane.d;
            if (dist < -radius)
                return false; // outside
        }
        return true; // potentially visible or intersecting
    }

    INLINE static bool sphereVisibleSIMD(const glm::vec3& center, const float radius, Frustum& frustum) {

        for (size_t i = 0; i < 6; ++i) {
            auto plane = &frustum.planes[i].x;
            __m128 vplane = _mm_loadu_ps(plane);           // [nx, ny, nz, d]
            __m128 vcenter = _mm_set_ps(1.0f, center.z, center.y, center.x); // [x, y, z, 1]
            // Dot product for first 3 components, then add d (plane[3])
            __m128 dp = _mm_dp_ps(vplane, vcenter, 0x71);   // Only x, y, z
            float dist;
            _mm_store_ss(&dist, dp);
            dist += plane[3]; // add d

            if (dist >= -radius)
                return false;
        }
        return true;

    }

    INLINE static bool __aabbVisibleSIMDPlane_mul(const float* plane,
                                                 const glm::vec3& mn,
                                                 const glm::vec3& mx) {
        const __m128 n_d = _mm_loadu_ps(plane);                 // [nx, ny, nz, d]
        const __m128 vmin = _mm_set_ps(0.0f, mn.z, mn.y, mn.x);  // [x,y,z,0]
        const __m128 vmax = _mm_set_ps(0.0f, mx.z, mx.y, mx.x);  // [x,y,z,0]

        // choose max where n>=0, else min  (p-vertex)
        const __m128 mask = _mm_cmpge_ps(n_d, _mm_setzero_ps());
        const __m128 p = _mm_or_ps(_mm_and_ps(mask, vmax),
                                   _mm_andnot_ps(mask, vmin));

        // dot3 = sum(x,y,z) of (n * p)  -- SSE2 reduction
        __m128 mul = _mm_mul_ps(n_d, p);                        // [nx*px, ny*py, nz*pz, 0]
        __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3)); // [nz*pz, ny*py, nx*px, 0]
        __m128 sum = _mm_add_ps(mul, shuf);                     // [(x+z), (y+0), (z+x), 0]
        shuf = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 0, 3, 2)); // [(y+0), (x+z), ..., ...]
        __m128 dpv = _mm_add_ss(sum, shuf);                     // [x+y+z, ...]
        float dist = _mm_cvtss_f32(dpv) + plane[3];             // + d

        return dist >= 0.0f;  // flip sign if your plane convention is opposite
    }

    INLINE static bool aabbVisible(const glm::vec3& min, const glm::vec3& max, const Frustum& frustum) {
        //glm::vec3 mn{ box.frontTopLeft.x,   box.backBottomRight.y, box.backBottomRight.z };
        //glm::vec3 mx{ box.backBottomRight.x, box.frontTopLeft.y,   box.frontTopLeft.z };
        for (size_t i = 0; i < 6; ++i) {
            if (!__aabbVisibleSIMDPlane_mul(frustum.planes[i].raw, min, max))
                return false;
        }
        return true;
    }

    //INLINE static bool obbVisible(const OBB& obb, const Frustum& frustum) {
    //    glm::vec3 center = (obb.frontTopLeft + obb.backBottomRight) * 0.5f;
    //    glm::vec3 extents = glm::abs(obb.backBottomRight - obb.frontTopLeft) * 0.5f;

    //    // Build orientation axes
    //    glm::mat3 rot = glm::mat3_cast(obb.rotation);
    //    glm::vec3 axes[3] = {
    //        rot[0], rot[1], rot[2]
    //    };
    //    for (int i = 0; i < 6; ++i) {
    //        const auto& plane = frustum.planes[i];
    //        // Project the OBB's half-extents onto the plane normal
    //        float r =
    //            extents.x * std::abs(glm::dot(plane.normal, axes[0])) +
    //            extents.y * std::abs(glm::dot(plane.normal, axes[1])) +
    //            extents.z * std::abs(glm::dot(plane.normal, axes[2]));

    //        float dist = glm::dot(plane.normal, center) + plane.d;
    //        if (dist < -r)
    //            return false; // OBB is fully outside
    //    }
    //    return true;
    //}
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