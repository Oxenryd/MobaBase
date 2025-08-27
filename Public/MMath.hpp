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
    constexpr const float fSQR2Inv = 0.707106781f;
    constexpr const double dSQR2Inv = 0.70710678118655;
    static const glm::mat4x4 IDENTITY_MAT = glm::mat4x4{ 1 };


    static const glm::mat4x4& identityMat4() { 
        return IDENTITY_MAT;
    }


    INLINE static glm::quat quatFromRotationVector(const glm::vec3& w) {
        // w direction = axis, |w| = angle
        float theta = glm::length(w);
        if (theta < 1e-12f) return glm::quat(1, 0, 0, 0);
        glm::vec3 axis = w / theta;
        return glm::angleAxis(theta, axis);
    }

    INLINE static glm::quat quatDelta(const glm::quat& qFrom, const glm::quat& qTo) {
        // Make sure we take the shortest arc by aligning hemisphere
        glm::quat a = qFrom;
        glm::quat b = qTo;
        if (glm::dot(a, b) < 0.0f) b = -b;
        // qDelta * a = b  =>  qDelta = b * conjugate(a)
        return glm::normalize(b * glm::conjugate(a));
    }

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


    // Build 4 TRS matrices simultaneously using AVX2
    INLINE void build_trs_matrices_avx2(
        const glm::vec3* positions,    // [4] positions
        const glm::quat* rotations,    // [4] quaternions  
        const glm::vec3* scales,       // [4] scales
        glm::mat4* output_matrices     // [4] output matrices
    ) {
        // Load 4 positions
        __m256 pos_x = _mm256_set_ps(0, positions[3].x, positions[2].x, positions[1].x,
                                     0, positions[0].x, positions[3].x, positions[2].x);
        __m256 pos_y = _mm256_set_ps(0, positions[3].y, positions[2].y, positions[1].y,
                                     0, positions[0].y, positions[3].y, positions[2].y);
        __m256 pos_z = _mm256_set_ps(0, positions[3].z, positions[2].z, positions[1].z,
                                     0, positions[0].z, positions[3].z, positions[2].z);

        // Load 4 quaternions
        __m256 qx = _mm256_set_ps(rotations[3].x, rotations[2].x, rotations[1].x, rotations[0].x,
                                  rotations[3].x, rotations[2].x, rotations[1].x, rotations[0].x);
        __m256 qy = _mm256_set_ps(rotations[3].y, rotations[2].y, rotations[1].y, rotations[0].y,
                                  rotations[3].y, rotations[2].y, rotations[1].y, rotations[0].y);
        __m256 qz = _mm256_set_ps(rotations[3].z, rotations[2].z, rotations[1].z, rotations[0].z,
                                  rotations[3].z, rotations[2].z, rotations[1].z, rotations[0].z);
        __m256 qw = _mm256_set_ps(rotations[3].w, rotations[2].w, rotations[1].w, rotations[0].w,
                                  rotations[3].w, rotations[2].w, rotations[1].w, rotations[0].w);

        // Load 4 scales
        __m256 sx = _mm256_set_ps(scales[3].x, scales[2].x, scales[1].x, scales[0].x,
                                  scales[3].x, scales[2].x, scales[1].x, scales[0].x);
        __m256 sy = _mm256_set_ps(scales[3].y, scales[2].y, scales[1].y, scales[0].y,
                                  scales[3].y, scales[2].y, scales[1].y, scales[0].y);
        __m256 sz = _mm256_set_ps(scales[3].z, scales[2].z, scales[1].z, scales[0].z,
                                  scales[3].z, scales[2].z, scales[1].z, scales[0].z);

        // Constants
        static const __m256 one = _mm256_set1_ps(1.0f);
        static const __m256 two = _mm256_set1_ps(2.0f);
        static const __m256 zero = _mm256_setzero_ps();

        // Quaternion to matrix conversion
        __m256 x2 = _mm256_mul_ps(qx, two);
        __m256 y2 = _mm256_mul_ps(qy, two);
        __m256 z2 = _mm256_mul_ps(qz, two);

        __m256 xx = _mm256_mul_ps(qx, x2);
        __m256 xy = _mm256_mul_ps(qx, y2);
        __m256 xz = _mm256_mul_ps(qx, z2);
        __m256 yy = _mm256_mul_ps(qy, y2);
        __m256 yz = _mm256_mul_ps(qy, z2);
        __m256 zz = _mm256_mul_ps(qz, z2);
        __m256 wx = _mm256_mul_ps(qw, x2);
        __m256 wy = _mm256_mul_ps(qw, y2);
        __m256 wz = _mm256_mul_ps(qw, z2);

        // Build rotation matrix with scale applied
        __m256 m00 = _mm256_mul_ps(_mm256_sub_ps(one, _mm256_add_ps(yy, zz)), sx);
        __m256 m01 = _mm256_mul_ps(_mm256_add_ps(xy, wz), sx);
        __m256 m02 = _mm256_mul_ps(_mm256_sub_ps(xz, wy), sx);

        __m256 m10 = _mm256_mul_ps(_mm256_sub_ps(xy, wz), sy);
        __m256 m11 = _mm256_mul_ps(_mm256_sub_ps(one, _mm256_add_ps(xx, zz)), sy);
        __m256 m12 = _mm256_mul_ps(_mm256_add_ps(yz, wx), sy);

        __m256 m20 = _mm256_mul_ps(_mm256_add_ps(xz, wy), sz);
        __m256 m21 = _mm256_mul_ps(_mm256_sub_ps(yz, wx), sz);
        __m256 m22 = _mm256_mul_ps(_mm256_sub_ps(one, _mm256_add_ps(xx, yy)), sz);

        // Store results to output matrices
        // This is the tricky part - we need to deinterleave and store 4 separate matrices
        alignas(32) float temp[8];

        for (int i = 0; i < 4; ++i) {
            // Extract values for matrix i
            _mm256_store_ps(temp, m00); output_matrices[i][0][0] = temp[i];
            _mm256_store_ps(temp, m01); output_matrices[i][0][1] = temp[i];
            _mm256_store_ps(temp, m02); output_matrices[i][0][2] = temp[i];
            output_matrices[i][0][3] = 0.0f;

            _mm256_store_ps(temp, m10); output_matrices[i][1][0] = temp[i];
            _mm256_store_ps(temp, m11); output_matrices[i][1][1] = temp[i];
            _mm256_store_ps(temp, m12); output_matrices[i][1][2] = temp[i];
            output_matrices[i][1][3] = 0.0f;

            _mm256_store_ps(temp, m20); output_matrices[i][2][0] = temp[i];
            _mm256_store_ps(temp, m21); output_matrices[i][2][1] = temp[i];
            _mm256_store_ps(temp, m22); output_matrices[i][2][2] = temp[i];
            output_matrices[i][2][3] = 0.0f;

            output_matrices[i][3][0] = positions[i].x;
            output_matrices[i][3][1] = positions[i].y;
            output_matrices[i][3][2] = positions[i].z;
            output_matrices[i][3][3] = 1.0f;
        }
    }

    INLINE glm::mat4 matrix_multiply_avx2(const glm::mat4& a, const glm::mat4& b) {
        alignas(32) glm::mat4 result;

        const float* pa = &a[0][0]; // column-major, 16 floats
        const float* pb = &b[0][0];

        // Load A columns {0,1} and {2,3} stacked into 256-bit vectors:
        // a01 = [a00 a10 a20 a30 | a01 a11 a21 a31]
        // a23 = [a02 a12 a22 a32 | a03 a13 a23 a33]
        __m256 a01 = _mm256_load_ps(pa + 0);
        __m256 a23 = _mm256_load_ps(pa + 8);

        // Duplicate each A column into both lanes so we can scale with different per-lane coeffs
        __m256 a0 = _mm256_permute2f128_ps(a01, a01, 0x00); // [col0 | col0]
        __m256 a1 = _mm256_permute2f128_ps(a01, a01, 0x11); // [col1 | col1]
        __m256 a2 = _mm256_permute2f128_ps(a23, a23, 0x00); // [col2 | col2]
        __m256 a3 = _mm256_permute2f128_ps(a23, a23, 0x11); // [col3 | col3]

        // Helper: build a 256-bit vector with low lane filled by lo (broadcast),
        // high lane filled by hi (broadcast).
        auto pair = [](__m128 lo, __m128 hi) {
            return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
            };

        // --- Compute result columns 0 & 1 in one go ---
        // Memory layout of B (column-major):
        // pb+0..3:  col0 (b00 b10 b20 b30)
        // pb+4..7:  col1 (b01 b11 b21 b31)
        __m256 b0_01 = pair(_mm_broadcast_ss(pb + 0), _mm_broadcast_ss(pb + 4));  // [b00 | b01]
        __m256 b1_01 = pair(_mm_broadcast_ss(pb + 1), _mm_broadcast_ss(pb + 5));  // [b10 | b11]
        __m256 b2_01 = pair(_mm_broadcast_ss(pb + 2), _mm_broadcast_ss(pb + 6));  // [b20 | b21]
        __m256 b3_01 = pair(_mm_broadcast_ss(pb + 3), _mm_broadcast_ss(pb + 7));  // [b30 | b31]

        __m256 r01 = _mm256_mul_ps(a0, b0_01);
        r01 = _mm256_fmadd_ps(a1, b1_01, r01);
        r01 = _mm256_fmadd_ps(a2, b2_01, r01);
        r01 = _mm256_fmadd_ps(a3, b3_01, r01);
        _mm256_store_ps(&result[0][0], r01); // stores col0 (low lane) and col1 (high lane)

        // --- Compute result columns 2 & 3 in one go ---
        // pb+8..11: col2 (b02 b12 b22 b32)
        // pb+12..15: col3 (b03 b13 b23 b33)
        __m256 b0_23 = pair(_mm_broadcast_ss(pb + 8), _mm_broadcast_ss(pb + 12)); // [b02 | b03]
        __m256 b1_23 = pair(_mm_broadcast_ss(pb + 9), _mm_broadcast_ss(pb + 13)); // [b12 | b13]
        __m256 b2_23 = pair(_mm_broadcast_ss(pb + 10), _mm_broadcast_ss(pb + 14)); // [b22 | b23]
        __m256 b3_23 = pair(_mm_broadcast_ss(pb + 11), _mm_broadcast_ss(pb + 15)); // [b32 | b33]

        __m256 r23 = _mm256_mul_ps(a0, b0_23);
        r23 = _mm256_fmadd_ps(a1, b1_23, r23);
        r23 = _mm256_fmadd_ps(a2, b2_23, r23);
        r23 = _mm256_fmadd_ps(a3, b3_23, r23);
        _mm256_store_ps(&result[2][0], r23); // stores col2 (low lane) and col3 (high lane)

        return result;
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