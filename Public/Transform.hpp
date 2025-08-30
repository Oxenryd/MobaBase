#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "ArenaAllocator.hpp"
#include <span>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bits.hpp"

#include "MMath.hpp"
#include "ObjectState.hpp"
#include "GlobalMacros.h"
#include "TransformComponent.hpp"

static const glm::vec3 DIR_FORWARD{ 0.0f, 0.0f, -1.0f };
static const glm::vec3 DIR_RIGHT{ 1.0f, 0.0f, 0.0f };
static const glm::vec3 DIR_UP{ 0.0f, 1.0f, 0.0f };
static const uint8_t TRANSLATION = 0;
static const uint8_t ROTATION = 1;
static const uint8_t SCALE = 2;
static const uint8_t TRANSFORM = 3;
struct Transform
{
	
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

	TransformComponent& _markDirty(uint8_t what);

	void _rotateLocalSIMD(const glm::vec3& rotDeltaLocal);



public:
	~Transform() = default;
	Transform() = delete;
	Transform(ArenaRegistry* registry, entt::entity entity)
		: m_reg{registry}, m_entity{entity} {}
	Transform(const Transform& other) {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
	}
	Transform& operator=(const Transform& rhs) {
		if (this == &rhs)
			return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	Transform(Transform&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	Transform& operator=(Transform&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	bool operator==(const Transform& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}

	INLINE operator TransformComponent& () {
		return m_reg->get<TransformComponent>(m_entity);
	}
	INLINE operator const TransformComponent& () const {
		return m_reg->get<TransformComponent>(m_entity);
	}
	
	glm::vec3& modifyPosition();
	glm::quat& modifyRotation();
	glm::vec3& modifyScale();
	INLINE StateField& modifyState() {
		return _markDirty(UINT8_INVALID).state;
	}
	INLINE glm::quat& setFromEuler(const glm::vec3& anglesDeg) {
		return modifyRotation() = glm::quat{ glm::radians(anglesDeg) };
	}

	const glm::vec3& position() const;
	const glm::vec3& scale() const;
	const glm::quat& rotation() const;
	INLINE const StateField& state() const { return m_reg->get<TransformComponent>(m_entity).state; }
	INLINE const glm::vec3 eulerAngles() const { return glm::eulerAngles(rotation()); }

	const glm::mat4x4& localToWorld() const;
	const glm::mat4x4 worldToLocal() const;
	INLINE glm::mat3 normalMatrix() const {

		glm::mat3 rotScale = glm::mat3_cast(rotation());
		auto& s = scale();
		rotScale[0] /= (s.x * s.x);
		rotScale[1] /= (s.y * s.y);
		rotScale[2] /= (s.z * s.z);
		return rotScale;
	}
	INLINE glm::mat4x4 translationMatrix() const {
		return glm::translate(glm::mat4(1.0f), position());
	}
	INLINE glm::mat4x4 scaleMatrix() const {
		return glm::scale(glm::mat4(1.0f), scale());
	}
	INLINE glm::mat4x4 rotationMatrix() const {
		return glm::mat4_cast(rotation());
	}
	INLINE glm::mat4 translationRotationMatrix() const {
		return MMath::composeTRS(position(), rotation(), glm::vec3(1.0f));
	}

	bool isValid() const {
		return m_reg && m_reg->valid(m_entity) && m_reg->all_of<TransformComponent>(m_entity);
	}

	Transform& lerpBetween(const Transform& a, const Transform& b, float t);

	INLINE glm::vec3& lerpPosition(const glm::vec3& a, const glm::vec3& b, float t) {
		return modifyPosition() = glm::mix(a, b, t);
	}

	INLINE glm::quat& lerpRotation(const glm::quat& a, const glm::quat& b, float t) {
		return modifyRotation() = glm::slerp(a, b, t);
	}

	INLINE glm::vec3& lerpScale(const glm::vec3& a, const glm::vec3& b, float t) {
		return modifyScale() = glm::mix(a, b, t);
	}

	INLINE glm::vec3 forward() { return rotation() * DIR_FORWARD; }
	INLINE glm::vec3 right() { return rotation() * DIR_RIGHT; }
	INLINE glm::vec3 up() { return rotation() * DIR_UP; }

	void translate(const glm::vec3& direction);
	void rotateByVector(const glm::vec3& deltaVec);
	void rotate(const glm::vec3& deltaEuler);
	void rotate(const glm::quat& worldDelta);
	void rotateLocal(const glm::quat& deltaRotLocal);
	void rotateLocal(const glm::vec3& deltaRotLocal);
	void rotateLocalWorldYaw(const glm::vec3& rotDelta);

	void rotateAroundWorldAxis(const glm::vec3& axis, float angle);
	INLINE void lookAt(const glm::vec3& target, const glm::vec3& up = DIR_UP) {
		glm::vec3 forward = glm::normalize(target - position());
		auto& rot = modifyRotation();
		rot = glm::quatLookAt(forward, up);
	}

	INLINE glm::vec3 transformPoint(const glm::vec3& localPoint) const {
		glm::vec4 worldPoint = localToWorld() * glm::vec4(localPoint, 1.0f);
		return glm::vec3(worldPoint);
	}
	glm::vec3 transformDirection(const glm::vec3& localDirection) const;
	INLINE glm::vec3 transformNormal(const glm::vec3& localNormal) const {
		return normalMatrix() * localNormal;
	}

	std::string_view getTag() const;

	std::span<entt::entity> getChildrenEntities() const;
	std::optional<Transform> getParentTransform() const;

	void setParent(const Transform* parent);
	void setParent(const entt::entity& parent);
	void clearChildren();

	// Static
	static const glm::mat4x4& localToWorld(ArenaRegistry* registry, entt::entity entity);
	static glm::vec3& position(ArenaRegistry* registry, entt::entity entity);
	static glm::vec3 positionInvertY(ArenaRegistry* registry, entt::entity entity);
	static glm::vec3& scale(ArenaRegistry* registry, entt::entity entity);
	static glm::quat& rotation(ArenaRegistry* registry, entt::entity entity);
	INLINE static StateField& state(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).state;
	}
	static glm::mat4x4 composeTRS(ArenaRegistry* registry, entt::entity entity);
	static std::span<entt::entity> getChildrenEntities(ArenaRegistry* registry, entt::entity ofEntity);
	static std::optional<Transform> getParentTransform(ArenaRegistry* registry, entt::entity ofEntity);
};

#endif