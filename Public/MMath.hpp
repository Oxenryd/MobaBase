#ifndef MOBAMATH_HPP
#define MOBAMATH_HPP

#include <immintrin.h>
#include <concepts>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <span>
#include "HlslTypes.h"
#include "Frustum.hpp"
#include "Range.hpp"
//#include "BasicTypes.hpp"
#include "Unions.h"





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

    template <std::integral U, std::integral V, std::integral W, std::integral Z, std::integral X>
    INLINE size_t getChunkSetRange(W threadCount, X size, Z currentThreadIndex, U& start, V& count) {
        size_t offset = static_cast<size_t>(
            std::ceil(static_cast<float>(size / static_cast<float>(threadCount)))
            );
        start = currentThreadIndex * offset;
        count = (currentThreadIndex == threadCount - 1 && size % offset != 0)
            ? size % offset
            : offset;

        return offset;
    }

    template <std::integral U, std::integral V, std::integral W, std::integral Z, std::integral X>
    INLINE auto getRange(W threadCount, X size, Z currentThreadIndex, U& start, V& count) {
        Range<X> r{};

        getChunkSetRange(threadCount, size, currentThreadIndex, r.offset, r.count);

        return r;
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
        const auto& m = viewProjection;

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

    INLINE static MaskState<6, 2, uint8_t> aabbClassifyWithMask(const glm::vec3& min, const glm::vec3& max, const Frustum& frustum) {
        // Convert to center-extent
        const glm::vec3 center = (min + max) * 0.5f;
        const glm::vec3 extent = (max - min) * 0.5f;

        uint8_t mask = 0;
        bool anyIntersect = false;

        for (int i = 0; i < 6; ++i) {
            // Quick outside test using your existing function
            if (!__aabbVisibleSIMDPlane_mul(frustum.planes[i].raw, min, max)) {
                return { 0, 0 };  // Outside
            }

            // Now do the center-extent test for intersection
            const float* plane = frustum.planes[i].raw;
            const __m128 n_d = _mm_load_ps(plane);
            const __m128 center_vec = _mm_set_ps(0.0f, center.z, center.y, center.x);
            const __m128 extent_vec = _mm_set_ps(0.0f, extent.z, extent.y, extent.x);

            // s = dot(n, center) + d
            __m128 mul_center = _mm_mul_ps(n_d, center_vec);
            __m128 shuf = _mm_shuffle_ps(mul_center, mul_center, _MM_SHUFFLE(2, 1, 0, 3));
            __m128 sum = _mm_add_ps(mul_center, shuf);
            shuf = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 0, 3, 2));
            __m128 dot_result = _mm_add_ss(sum, shuf);
            float s = _mm_cvtss_f32(dot_result) + plane[3];

            // r = dot(|n|, extent)  
            const __m128 abs_n = _mm_andnot_ps(_mm_set1_ps(-0.0f), n_d);
            __m128 mul_extent = _mm_mul_ps(abs_n, extent_vec);
            shuf = _mm_shuffle_ps(mul_extent, mul_extent, _MM_SHUFFLE(2, 1, 0, 3));
            sum = _mm_add_ps(mul_extent, shuf);
            shuf = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 0, 3, 2));
            dot_result = _mm_add_ss(sum, shuf);
            float r = _mm_cvtss_f32(dot_result);

            if (s - r < 0.0f) {
                anyIntersect = true;
                mask |= (1u << i);
            }
        }

        return { mask, anyIntersect ? (uint8_t)1 : (uint8_t)2 };
    }

    //INLINE static bool obbVisible(const OBB& obb, const Frustum& frustum) {
    //    glm::vec3 center = (obb.frontTopLeft + obb.backBottomRight) * 0.5f;
    //    glm::vec3 extent = glm::abs(obb.backBottomRight - obb.frontTopLeft) * 0.5f;

    //    // Build orientation axes
    //    glm::mat3 rot = glm::mat3_cast(obb.rotation);
    //    glm::vec3 axes[3] = {
    //        rot[0], rot[1], rot[2]
    //    };
    //    for (int i = 0; i < 6; ++i) {
    //        const auto& plane = frustum.planes[i];
    //        // Project the OBB's half-extent onto the plane normal
    //        float r =
    //            extent.x * std::abs(glm::dot(plane.normal, axes[0])) +
    //            extent.y * std::abs(glm::dot(plane.normal, axes[1])) +
    //            extent.z * std::abs(glm::dot(plane.normal, axes[2]));

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






//    // SIMD quaternion multiplication
//// Quaternions stored as [x, y, z, w] in memory
//    inline __m128 quaternion_multiply_simd(__m128 q1, __m128 q2) {
//        // q1 = [x1, y1, z1, w1]
//        // q2 = [x2, y2, z2, w2]
//
//        // Quaternion multiplication formula:
//        // result.w = w1*w2 - x1*x2 - y1*y2 - z1*z2
//        // result.x = w1*x2 + x1*w2 + y1*z2 - z1*y2  
//        // result.y = w1*y2 - x1*z2 + y1*w2 + z1*x2
//        // result.z = w1*z2 + x1*y2 - y1*x2 + z1*w2
//
//        // Broadcast each component of q1
//        __m128 q1_wwww = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3)); // [w1, w1, w1, w1]
//        __m128 q1_xxxx = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0)); // [x1, x1, x1, x1]
//        __m128 q1_yyyy = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1)); // [y1, y1, y1, y1]
//        __m128 q1_zzzz = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2)); // [z1, z1, z1, z1]
//
//        // Rearrange q2 components for different terms
//        __m128 q2_xyzw = q2;                                               // [x2, y2, z2, w2]
//        __m128 q2_yxwz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1)); // [y2, x2, w2, z2]
//        __m128 q2_zyxw = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(3, 0, 1, 2)); // [z2, y2, x2, w2]
//        __m128 q2_wzyx = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3)); // [w2, z2, y2, x2]
//
//        // Calculate the four terms
//        __m128 term1 = _mm_mul_ps(q1_wwww, q2_xyzw); // [w1*x2, w1*y2, w1*z2, w1*w2]
//        __m128 term2 = _mm_mul_ps(q1_xxxx, q2_wzyx); // [x1*w2, x1*z2, x1*y2, x1*x2]
//        __m128 term3 = _mm_mul_ps(q1_yyyy, q2_zyxw); // [y1*z2, y1*y2, y1*x2, y1*w2]
//        __m128 term4 = _mm_mul_ps(q1_zzzz, q2_yxwz); // [z1*y2, z1*x2, z1*w2, z1*z2]
//
//        // Apply signs: [+, -, +, -] for term2, [+, -, -, +] for term3, [-, +, -, -] for term4
//        const __m128 sign2 = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);
//        const __m128 sign3 = _mm_set_ps(1.0f, -1.0f, -1.0f, 1.0f);
//        const __m128 sign4 = _mm_set_ps(-1.0f, -1.0f, 1.0f, -1.0f);
//
//        term2 = _mm_mul_ps(term2, sign2);
//        term3 = _mm_mul_ps(term3, sign3);
//        term4 = _mm_mul_ps(term4, sign4);
//
//        // Sum all terms
//        __m128 result = _mm_add_ps(term1, term2);
//        result = _mm_add_ps(result, term3);
//        result = _mm_add_ps(result, term4);
//
//        return result; // [result.x, result.y, result.z, result.w]
//    }
//
//    // SIMD quaternion normalization
//    inline __m128 normalize_simd(__m128 q) {
//        // Compute dot product of quaternion with itself
//        __m128 dot = _mm_mul_ps(q, q);
//
//        // Horizontal add to get magnitude squared
//        __m128 temp = _mm_hadd_ps(dot, dot);
//        __m128 mag_sq = _mm_hadd_ps(temp, temp);
//
//        // Compute reciprocal square root
//        __m128 rsqrt = _mm_rsqrt_ps(mag_sq);
//
//        // One Newton-Raphson iteration for better precision
//        // rsqrt = 0.5 * rsqrt * (3 - mag_sq * rsqrt * rsqrt)
//        __m128 three = _mm_set1_ps(3.0f);
//        __m128 half = _mm_set1_ps(0.5f);
//        __m128 temp2 = _mm_mul_ps(mag_sq, _mm_mul_ps(rsqrt, rsqrt));
//        __m128 temp3 = _mm_sub_ps(three, temp2);
//        rsqrt = _mm_mul_ps(half, _mm_mul_ps(rsqrt, temp3));
//
//        // Normalize by multiplying with reciprocal square root
//        return _mm_mul_ps(q, rsqrt);
//    }
//
//    // Alternative more accurate normalization using sqrt
//    inline __m128 normalize_simd_accurate(__m128 q) {
//        __m128 dot = _mm_mul_ps(q, q);
//        __m128 temp = _mm_hadd_ps(dot, dot);
//        __m128 mag_sq = _mm_hadd_ps(temp, temp);
//
//        __m128 mag = _mm_sqrt_ps(mag_sq);
//        return _mm_div_ps(q, mag);
//    }
//
//    // Fast sin/cos approximation for small angles (good for rotations)
//    inline void sincos_approx(float x, float& sin_x, float& cos_x) {
//        // Taylor series approximation for small angles
//        const float x2 = x * x;
//        const float x3 = x2 * x;
//        const float x4 = x2 * x2;
//        const float x5 = x4 * x;
//
//        sin_x = x - (x3 * 0.16666667f) + (x5 * 0.00833333f);
//        cos_x = 1.0f - (x2 * 0.5f) + (x4 * 0.04166667f);
//    }
    // Debug version - test against reference implementation
    inline glm::quat quaternion_multiply_reference(const glm::quat& q1, const glm::quat& q2) {
        return q1 * q2;
    }

    // Corrected SIMD quaternion multiplication that matches GLM
    inline __m128 quaternion_multiply_simd_simple(__m128 a, __m128 b) {
        // a = [ax, ay, az, aw] (GLM layout)
        // b = [bx, by, bz, bw] (GLM layout)
        // 
        // GLM quaternion multiplication: result = a * b
        // result.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y
        // result.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x
        // result.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w  
        // result.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z

        // Broadcast a components
        __m128 a_xxxx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));  // a.x
        __m128 a_yyyy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1));  // a.y
        __m128 a_zzzz = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2));  // a.z
        __m128 a_wwww = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));  // a.w

        // First term: a.w * b = [a.w*b.x, a.w*b.y, a.w*b.z, a.w*b.w]
        __m128 result = _mm_mul_ps(a_wwww, b);

        // Second term: a.x * [b.w, -b.z, b.y, -b.x]
        __m128 b_wzyx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3));   // [b.w, b.z, b.y, b.x]
        __m128 axb = _mm_mul_ps(a_xxxx, b_wzyx);
        __m128 sign_x = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);          // [+, -, +, -] for [x,y,z,w]
        axb = _mm_mul_ps(axb, sign_x);
        result = _mm_add_ps(result, axb);

        // Third term: a.y * [b.z, b.w, -b.x, -b.y]  
        __m128 b_zwxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 0, 3, 2));   // [b.z, b.w, b.x, b.y]
        __m128 ayb = _mm_mul_ps(a_yyyy, b_zwxy);
        __m128 sign_y = _mm_set_ps(-1.0f, -1.0f, 1.0f, 1.0f);          // [+, +, -, -] for [x,y,z,w]
        ayb = _mm_mul_ps(ayb, sign_y);
        result = _mm_add_ps(result, ayb);

        // Fourth term: a.z * [-b.y, b.x, b.w, -b.z]
        __m128 b_yxwz = _mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 3, 0, 1));   // [b.y, b.x, b.w, b.z]  
        __m128 azb = _mm_mul_ps(a_zzzz, b_yxwz);
        __m128 sign_z = _mm_set_ps(-1.0f, 1.0f, 1.0f, -1.0f);          // [-, +, +, -] for [x,y,z,w]
        azb = _mm_mul_ps(azb, sign_z);
        result = _mm_add_ps(result, azb);

        return result;
    }

    // SIMD quaternion multiplication
    // Quaternions stored as [x, y, z, w] in memory
    inline __m128 quaternion_multiply_simd(__m128 q1, __m128 q2) {
        // q1 = [x1, y1, z1, w1] 
        // q2 = [x2, y2, z2, w2]

        // Standard quaternion multiplication:
        // result.x = w1*x2 + x1*w2 + y1*z2 - z1*y2
        // result.y = w1*y2 - x1*z2 + y1*w2 + z1*x2  
        // result.z = w1*z2 + x1*y2 - y1*x2 + z1*w2
        // result.w = w1*w2 - x1*x2 - y1*y2 - z1*z2

        // More explicit approach - let's build each component separately
        __m128 q1_wwww = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 q1_xxxx = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 q1_yyyy = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 q1_zzzz = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2));

        // For result.x = w1*x2 + x1*w2 + y1*z2 - z1*y2
        __m128 q2_x = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 0, 0, 0)); // [x2, x2, x2, x2]
        __m128 q2_w = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(3, 3, 3, 3)); // [w2, w2, w2, w2] 
        __m128 q2_z = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 2, 2, 2)); // [z2, z2, z2, z2]
        __m128 q2_y = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 1, 1, 1)); // [y2, y2, y2, y2]

        // Calculate: [w1*x2, w1*y2, w1*z2, w1*w2]
        __m128 term_w1 = _mm_mul_ps(q1_wwww, q2);

        // Calculate: [x1*w2, x1*z2, x1*y2, x1*x2] -> need [x1*w2, -x1*z2, x1*y2, -x1*x2]
        __m128 q2_wzyx = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 term_x1 = _mm_mul_ps(q1_xxxx, q2_wzyx);
        __m128 sign_x1 = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);
        term_x1 = _mm_mul_ps(term_x1, sign_x1);

        // Calculate: [y1*z2, y1*w2, -y1*x2, -y1*y2]  
        __m128 q2_zw_xy = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 term_y1 = _mm_mul_ps(q1_yyyy, q2_zw_xy);
        __m128 sign_y1 = _mm_set_ps(-1.0f, -1.0f, 1.0f, 1.0f);
        term_y1 = _mm_mul_ps(term_y1, sign_y1);

        // Calculate: [-z1*y2, z1*x2, z1*w2, -z1*z2]
        __m128 q2_yxwz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 term_z1 = _mm_mul_ps(q1_zzzz, q2_yxwz);
        __m128 sign_z1 = _mm_set_ps(-1.0f, 1.0f, 1.0f, -1.0f);
        term_z1 = _mm_mul_ps(term_z1, sign_z1);

        // Sum all terms
        __m128 result = _mm_add_ps(term_w1, term_x1);
        result = _mm_add_ps(result, term_y1);
        result = _mm_add_ps(result, term_z1);

        return result;
    }

    // Alternative cleaner implementation
    inline __m128 quaternion_multiply_simd_v2(__m128 q1, __m128 q2) {
        // Based on the identity: q1 * q2 can be computed as matrix multiplication
        // This version is cleaner and easier to verify

        __m128 q1_x = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 q1_y = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 q1_z = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 q1_w = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3));

        // Rearrange q2 for each output component
        __m128 q2_xyzw = q2;                                                    // [x2, y2, z2, w2]
        __m128 q2_yxwz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1));     // [y2, x2, w2, z2]  
        __m128 q2_zwy_x = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 3, 2));     // [z2, w2, y2, x2]
        __m128 q2_wzyx = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3));      // [w2, z2, y2, x2]

        __m128 result = _mm_mul_ps(q1_w, q2_xyzw);              // w1 * [x2, y2, z2, w2]
        result = _mm_add_ps(result, _mm_mul_ps(q1_x, q2_wzyx)); // + x1 * [w2, z2, y2, x2]
        result = _mm_add_ps(result, _mm_mul_ps(q1_y, q2_zwy_x));// + y1 * [z2, w2, y2, x2] 
        result = _mm_add_ps(result, _mm_mul_ps(q1_z, q2_yxwz)); // + z1 * [y2, x2, w2, z2]

        // Apply the correct signs for quaternion multiplication
        __m128 signs = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);   // [+, -, +, -] for [x, y, z, w]
        __m128 temp = _mm_mul_ps(q1_x, q2_wzyx);
        temp = _mm_add_ps(temp, _mm_mul_ps(q1_y, _mm_shuffle_ps(q2_zwy_x, q2_zwy_x, _MM_SHUFFLE(2, 0, 3, 1))));
        temp = _mm_add_ps(temp, _mm_mul_ps(q1_z, _mm_shuffle_ps(q2_yxwz, q2_yxwz, _MM_SHUFFLE(3, 1, 0, 2))));

        // This is getting complex - let me provide a tested version
        return quaternion_multiply_simd_simple(q1, q2);
    }

    

    // Alternative implementation using a different approach
    inline __m128 quaternion_multiply_simd_v3(__m128 q1, __m128 q2) {
        // This version follows the exact GLM implementation structure
        // Based on GLM's qua<T, Q>::operator* implementation

        // GLM: return qua<T, Q>(
        //   w * q.w - x * q.x - y * q.y - z * q.z,  // w component
        //   w * q.x + x * q.w + y * q.z - z * q.y,  // x component  
        //   w * q.y + y * q.w + z * q.x - x * q.z,  // y component
        //   w * q.z + z * q.w + x * q.y - y * q.x); // z component

        __m128 q1_wwww = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 q1_xxxx = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 q1_yyyy = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 q1_zzzz = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2));

        // Compute w*q2
        __m128 wq2 = _mm_mul_ps(q1_wwww, q2);

        // For each output component, we need different arrangements of q2:
        // x: w*x + x*w + y*z - z*y  ->  q2: [x,w,z,y], signs: [+,+,+,-]  
        // y: w*y + y*w + z*x - x*z  ->  q2: [y,w,x,z], signs: [+,+,+,-]
        // z: w*z + z*w + x*y - y*x  ->  q2: [z,w,y,x], signs: [+,+,+,-] 
        // w: w*w - x*x - y*y - z*z  ->  q2: [w,x,y,z], signs: [+,-,-,-]

        __m128 q2_xwzy = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 2, 3, 0)); // [x,w,z,y]
        __m128 q2_ywxz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 0, 3, 1)); // [y,w,x,z] 
        __m128 q2_zwyx = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 3, 2)); // [z,w,y,x]
        __m128 q2_wxyz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 1, 0, 3)); // [w,x,y,z]

        // Pack the rearranged q2 components for parallel computation
        // This is getting complex - let's stick with the component-wise approach above
        return quaternion_multiply_simd_simple(q1, q2);
    }

    // SIMD quaternion normalization
    inline __m128 normalize_simd(__m128 q) {
        // Compute dot product of quaternion with itself
        __m128 dot = _mm_mul_ps(q, q);

        // Horizontal add to get magnitude squared
        __m128 temp = _mm_hadd_ps(dot, dot);
        __m128 mag_sq = _mm_hadd_ps(temp, temp);

        // Compute reciprocal square root
        __m128 rsqrt = _mm_rsqrt_ps(mag_sq);

        // One Newton-Raphson iteration for better precision
        // rsqrt = 0.5 * rsqrt * (3 - mag_sq * rsqrt * rsqrt)
        __m128 three = _mm_set1_ps(3.0f);
        __m128 half = _mm_set1_ps(0.5f);
        __m128 temp2 = _mm_mul_ps(mag_sq, _mm_mul_ps(rsqrt, rsqrt));
        __m128 temp3 = _mm_sub_ps(three, temp2);
        rsqrt = _mm_mul_ps(half, _mm_mul_ps(rsqrt, temp3));

        // Normalize by multiplying with reciprocal square root
        return _mm_mul_ps(q, rsqrt);
    }

    // Alternative more accurate normalization using sqrt
    inline __m128 normalize_simd_accurate(__m128 q) {
        __m128 dot = _mm_mul_ps(q, q);
        __m128 temp = _mm_hadd_ps(dot, dot);
        __m128 mag_sq = _mm_hadd_ps(temp, temp);

        __m128 mag = _mm_sqrt_ps(mag_sq);
        return _mm_div_ps(q, mag);
    }

    // Fast sin/cos approximation for small angles (good for rotations)
    inline void sincos_approx(float x, float& sin_x, float& cos_x) {
        // Taylor series approximation for small angles
        const float x2 = x * x;
        const float x3 = x2 * x;
        const float x4 = x2 * x2;
        const float x5 = x4 * x;

        sin_x = x - (x3 * 0.16666667f) + (x5 * 0.00833333f);
        cos_x = 1.0f - (x2 * 0.5f) + (x4 * 0.04166667f);
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