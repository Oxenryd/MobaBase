#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <cstdint>
#include <immintrin.h>

#include "GameObject.hpp"
#include "RenderTarget.hpp"
#include "Bits.hpp"
#include "HlslTypes.h"
#include "BasicTypes.hpp"





class SceneRenderSystem;
struct CamIndex;
class Camera : public GameObject
{
    friend Engine;
    friend SceneRenderSystem;

private:
	bool m_viewDirty = true;
	bool m_projDirty = true;

	LayerMask m_layerMask{ 1 };
    CameraData m_camData{};


    INLINE void _checkDirty() {
        auto& world = worldToView();
        auto& proj = projection();
    }

public:
	virtual ~Camera() {
        if (transform().isValid()) {
            auto& comp = m_reg->get<TransformComponent>(m_entity);
            comp.onDirtyCallback = nullptr;
            comp.callbackUserData = nullptr;
        }
    }
    Camera(CameraData& initData) :
        GameObject{},
        m_camData{initData}
    {}
    Camera() :
        GameObject{}
    {

    }

    INLINE const glm::mat4& worldToView() {
        if (m_viewDirty) {

            auto transform = this->transform();

            const glm::vec3& pos = transform.position();
            const glm::quat& rot = transform.rotation();

            glm::quat invRot = glm::conjugate(rot);
            glm::vec3 invPos = -(invRot * pos);

            glm::mat4 rotMatrix = glm::mat4_cast(invRot);
            rotMatrix[3] = glm::vec4(invPos, 1.0f);

            m_camData.cameraPosition = glm::vec4{ pos, 1.0f };

            m_camData.view = rotMatrix;
            m_viewDirty = false;
        }
        return m_camData.view;
    }
    INLINE glm::mat4 viewToWorld() const {
        auto transform = this->transform();
        
        return transform.translationRotationMatrix();
    }

    INLINE const glm::mat4& projection() {
        if (m_projDirty) {
            m_camData.proj = glm::perspectiveRH_ZO(
                glm::radians(m_camData.vFov),
                m_camData.aspectRatio,
                m_camData.nearPlane,
                m_camData.farPlane
            );
            m_camData.proj[1][1] *= -1;


            m_projDirty = false;
        }
        return m_camData.proj;
    }

    INLINE glm::vec3 worldToViewPoint(const glm::vec3& worldPoint) {
        glm::vec4 viewPoint = worldToView() * glm::vec4(worldPoint, 1.0f);
        return glm::vec3(viewPoint);
    }
    INLINE glm::vec3 viewToWorldPoint(const glm::vec3& viewPoint) const {
        glm::vec4 worldPoint = viewToWorld() * glm::vec4(viewPoint, 1.0f);
        return glm::vec3(worldPoint);
    }
    INLINE glm::vec3 worldToViewDirection(const glm::vec3& worldDir) {
        glm::vec4 viewDir = worldToView() * glm::vec4(worldDir, 0.0f);
        return glm::vec3(viewDir);
    }
    INLINE glm::vec3 viewToWorldDirection(const glm::vec3& viewDir) const {
        glm::vec4 worldDir = viewToWorld() * glm::vec4(viewDir, 0.0f);
        return glm::vec3(worldDir);
    }
    INLINE glm::mat4 viewProjection() {
        return projection() * worldToView();
    }


    INLINE void setFOV(float fov) {
        m_camData.vFov = fov;
        m_projDirty = true;
    }
    INLINE void setAspectRatio(float aspect) {
        m_camData.aspectRatio = aspect;
        m_projDirty = true;
    }
    INLINE void setNearFar(float nearPlane, float farPlane) {
        m_camData.nearPlane = nearPlane;
        m_camData.farPlane = farPlane;
        m_projDirty = true;
    }
    INLINE glm::vec3 getPosition() const { return m_camData.cameraPosition; }

    INLINE void translate(const glm::vec3& direction) {
        transform().translate(direction);
    }
    INLINE void rotate(const glm::vec3& eulerDegreesDelta) {
        transform().rotate(eulerDegreesDelta);
    }
    INLINE void rotateLocal(const glm::vec3& eulerDegreesDelta) {
        transform().rotateLocal(eulerDegreesDelta);
    }

    INLINE float getFOV() const { return m_camData.vFov; }
    INLINE float getAspectRatio() const { return m_camData.aspectRatio; }
    INLINE float getNearPlane() const { return m_camData.nearPlane; }
    INLINE float getFarPlane() const { return m_camData.farPlane; }

    INLINE CameraData& cameraData() { 
        _checkDirty();
        return m_camData;
    }


    INLINE Frustum getFrustum(bool normalize = true) {
        Frustum f;
        auto m = viewProjection();

        // Left
        f.planes[Frustum::Left].normal.x = m[0][3] + m[0][0];
        f.planes[Frustum::Left].normal.y = m[1][3] + m[1][0];
        f.planes[Frustum::Left].normal.z = m[2][3] + m[2][0];
        f.planes[Frustum::Left].d = m[3][3] + m[3][0];
        // Right
        f.planes[Frustum::Right].normal.x = m[0][3] - m[0][0];
        f.planes[Frustum::Right].normal.y = m[1][3] - m[1][0];
        f.planes[Frustum::Right].normal.z = m[2][3] - m[2][0];
        f.planes[Frustum::Right].d = m[3][3] - m[3][0];
        // Bottom
        f.planes[Frustum::Bottom].normal.x = m[0][3] + m[0][1];
        f.planes[Frustum::Bottom].normal.y = m[1][3] + m[1][1];
        f.planes[Frustum::Bottom].normal.z = m[2][3] + m[2][1];
        f.planes[Frustum::Bottom].d = m[3][3] + m[3][1];
        // Top
        f.planes[Frustum::Top].normal.x = m[0][3] - m[0][1];
        f.planes[Frustum::Top].normal.y = m[1][3] - m[1][1];
        f.planes[Frustum::Top].normal.z = m[2][3] - m[2][1];
        f.planes[Frustum::Top].d = m[3][3] - m[3][1];
        // Near
        f.planes[Frustum::Near].normal.x = m[0][3] + m[0][2];
        f.planes[Frustum::Near].normal.y = m[1][3] + m[1][2];
        f.planes[Frustum::Near].normal.z = m[2][3] + m[2][2];
        f.planes[Frustum::Near].d = m[3][3] + m[3][2];
        // Far
        f.planes[Frustum::Far].normal.x = m[0][3] - m[0][2];
        f.planes[Frustum::Far].normal.y = m[1][3] - m[1][2];
        f.planes[Frustum::Far].normal.z = m[2][3] - m[2][2];
        f.planes[Frustum::Far].d = m[3][3] - m[3][2];

        //printf("\n\nSCALAR:\n");

        //for (int i = 0; i < 6; ++i) {
        //    printf("plane[%d]: %f %f %f %f\n", i, f.planes[i].raw[0], f.planes[i].raw[1], f.planes[i].raw[2], f.planes[i].raw[3]);
        //}

        // Optionally normalize the planes
        if (normalize) {
            for (int i = 0; i < 6; ++i) {
                float len = glm::length(f.planes[i].normal);
                f.planes[i].normal /= len;
                f.planes[i].d /= len;
            }
        }

        return f;
    }

    INLINE Frustum getFrustumSIMD(bool normalize = true) {

        Frustum f;
        auto m = viewProjection();

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


    INLINE static bool sphereVisible(const BSphere& s, Frustum& frustum) {
        for (int i = 0; i < 6; ++i) {
            const auto& plane = frustum.planes[i];
            float dist = glm::dot(plane.normal, s.center) + plane.d;
            if (dist < -s.radius)
                return false; // outside
        }
        return true; // potentially visible or intersecting
    }

    INLINE static bool sphereVisibleSIMD(const BSphere& s, Frustum& frustum) {

        for (size_t i = 0; i < 6; ++i) {
            auto plane = &frustum.planes[i].x;
            __m128 vplane = _mm_loadu_ps(plane);           // [nx, ny, nz, d]
            __m128 vcenter = _mm_set_ps(1.0f, s.center.z, s.center.y, s.center.x); // [x, y, z, 1]
            // Dot product for first 3 components, then add d (plane[3])
            __m128 dp = _mm_dp_ps(vplane, vcenter, 0x71);   // Only x, y, z
            float dist;
            _mm_store_ss(&dist, dp);
            dist += plane[3]; // add d

            if (dist >= -s.radius)
                return false;
        }
        return true;

    }

    INLINE static bool aabbVisible(const AABB& box, const Frustum& frustum) {

        // For each plane, check the "most negative" vertex
        for (int i = 0; i < 6; ++i) {
            glm::vec3 p;
            p.x = (frustum.planes[i].normal.x >= 0) ? box.frontTopLeft.x : box.backBottomRight.x;
            p.y = (frustum.planes[i].normal.y >= 0) ? box.frontTopLeft.y : box.backBottomRight.y;
            p.z = (frustum.planes[i].normal.z >= 0) ? box.frontTopLeft.z : box.backBottomRight.z;
            if (glm::dot(frustum.planes[i].normal, p) + frustum.planes[i].d > 0)
                continue;
            // If the most outside vertex is inside, check next plane
            // If not, completely outside!
            return false;
        }
        return true;
    }

private:
    INLINE static bool _aabbVisibleSIMDPlane(const float* plane, const glm::vec3& min, const glm::vec3& max) {
        __m128 vplane = _mm_loadu_ps(plane); // [nx, ny, nz, d]
        __m128 vmin = _mm_set_ps(0.0f, min.z, min.y, min.x);
        __m128 vmax = _mm_set_ps(0.0f, max.z, max.y, max.x);
        // For each axis, select min or max based on sign of plane normal
        __m128 mask = _mm_cmpge_ps(vplane, _mm_setzero_ps()); // >=0? mask 0xFFFFFFFF else 0x0
        __m128 p = _mm_or_ps(_mm_and_ps(mask, vmin), _mm_andnot_ps(mask, vmax)); // blend
        // Dot product
        __m128 dp = _mm_dp_ps(vplane, p, 0x71); // x,y,z
        float dist;
        _mm_store_ss(&dist, dp);
        dist += plane[3];
        return dist >= 0.0f;
    }
public:
    INLINE static bool aabbVisibleSIMD(const AABB& box, Frustum& frustum) {
        for (size_t i = 0; i < 6; ++i) {
            if (!_aabbVisibleSIMDPlane(frustum.planes[i].raw, box.frontTopLeft, box.backBottomRight))
                return false;
        }
        return true;
    }

    INLINE static bool obbVisible(const OBB& obb, const Frustum& frustum) {
        glm::vec3 center = (obb.frontTopLeft + obb.backBottomRight) * 0.5f;
        glm::vec3 extents = glm::abs(obb.backBottomRight - obb.frontTopLeft) * 0.5f;

        // Build orientation axes
        glm::mat3 rot = glm::mat3_cast(obb.rotation);
        glm::vec3 axes[3] = {
            rot[0], rot[1], rot[2]
        };
        for (int i = 0; i < 6; ++i) {
            const auto& plane = frustum.planes[i];
            // Project the OBB's half-extents onto the plane normal
            float r =
                extents.x * std::abs(glm::dot(plane.normal, axes[0])) +
                extents.y * std::abs(glm::dot(plane.normal, axes[1])) +
                extents.z * std::abs(glm::dot(plane.normal, axes[2]));

            float dist = glm::dot(plane.normal, center) + plane.d;
            if (dist < -r)
                return false; // OBB is fully outside
        }
        return true;
    }

    static Camera* getCamera(uint16_t sceneIndex, uint32_t camIndex);
    static Camera* getCamera(const CamIndex& camIndex);
};


#endif