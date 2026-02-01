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
    Frustum m_cachedFrustum;

	LayerMask m_layerMask{ 1 };
    CameraData m_camData{};


    INLINE void _checkDirty() {
        [[maybe_unused]] auto& world = worldToView();
        [[maybe_unused]] auto& proj = projection();
    }

public:
	virtual ~Camera() {
        if (transform().isValid()) {
            auto& comp = m_reg->get<TransformComponent>(m_entity);
            comp.onDirtyCallback = nullptr;
            comp.callbackUserData = nullptr;
        }
    }
    explicit Camera(const CameraData& initData) :
        GameObject{},
        m_camData{initData}
    {}
    Camera() :
        GameObject{}
    {

    }

    const glm::mat4& worldToView() {
        if (m_viewDirty) {

            const auto transform = this->transform();

            const glm::vec3& pos = transform.position();
            const glm::quat& rot = transform.rotation();

            const glm::quat invRot = glm::conjugate(rot);
            const glm::vec3 invPos = -(invRot * pos);

            glm::mat4 rotMatrix = glm::mat4_cast(invRot);
            rotMatrix[3] = glm::vec4(invPos, 1.0f);

            m_camData.cameraPosition[0] = pos[0];// = glm::vec4{ pos, 1.0f };
            m_camData.cameraPosition[1] = pos[1];
            m_camData.cameraPosition[2] = pos[2];

            m_camData.view = rotMatrix;
            m_viewDirty = false;

            m_cachedFrustum = MMath::getFrustum(viewProjection());
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

            m_camData.invProj = glm::mat4(0.0f);
            m_camData.invProj[0][0] = 1.0f / m_camData.proj[0][0];
            m_camData.invProj[1][1] = 1.0f / m_camData.proj[1][1];
            m_camData.invProj[2][3] = 1.0f;
            m_camData.invProj[3][2] = 1.0f / m_camData.proj[2][3];
            m_camData.invProj[3][3] = -m_camData.proj[2][2] / m_camData.proj[2][3];
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
    INLINE glm::mat4& inverseProjection() const {
        return const_cast<glm::mat4&>(m_camData.invProj);
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
    [[nodiscard]] INLINE glm::vec3 getPosition() const {
	    return glm::make_vec3(m_camData.cameraPosition);
	}

    INLINE void translate(const glm::vec3& direction) {
        transform().translate(direction);
    }
    INLINE void rotate(const glm::vec3& eulerDegreesDelta) {
        transform().rotate(eulerDegreesDelta);
    }
    INLINE void rotateLocal(const glm::vec3& eulerDegreesDelta) {
        transform().rotateLocalWorldYaw(eulerDegreesDelta);
    }

    [[nodiscard]] INLINE float getFOV() const { return m_camData.vFov; }
    [[nodiscard]] INLINE float getAspectRatio() const { return m_camData.aspectRatio; }
    [[nodiscard]] INLINE float getNearPlane() const { return m_camData.nearPlane; }
    [[nodiscard]] INLINE float getFarPlane() const { return m_camData.farPlane; }

    INLINE CameraData& cameraData() { 
        _checkDirty();
        return m_camData;
    }

    [[nodiscard]] INLINE ZRow getZRow() const {
        auto trans = transform();
        ZRow z{};
	    const auto fw = trans.forward();
	    z.n[0] = fw[0];
	    z.n[1] = fw[1];
	    z.n[2] = fw[2];
        z.w = -glm::dot(z.getN(), trans.position());
        return z;
    }

    INLINE Frustum& getFrustum() {

        return m_cachedFrustum;
    }
    [[nodiscard]] INLINE Frustum& getFrustum() const {

        return const_cast<Frustum&>(m_cachedFrustum);
    }

    static Camera* getCamera(uint16_t sceneIndex, uint32_t camIndex);
    static Camera* getCamera(const CamIndex& camIndex);
};


#endif