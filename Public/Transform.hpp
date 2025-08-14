#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "ArenaAllocator.hpp"
#include <span>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bits.hpp"

#include "MobaMath.hpp"
#include "ObjectState.hpp"
#include "GlobalMacros.h"
#include "TransformComponent.hpp"

static const glm::vec3 DIR_FORWARD{ 0.0f, 0.0f, -1.0f };
static const glm::vec3 DIR_RIGHT{ 1.0f, 0.0f, 0.0f };
static const glm::vec3 DIR_UP{ 0.0f, 1.0f, 0.0f };

struct Transform
{
	
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

	INLINE TransformComponent& _markDirty() {
		auto& comp = m_reg->get<TransformComponent>(m_entity);
		comp.state.setByEnum(ObjectState::DirtyTransform);
		if (comp.onDirtyCallback) {
			comp.onDirtyCallback(comp.callbackUserData);
		}
		return comp;
	}

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
	
	INLINE glm::vec3& modifyPosition() { 
		return _markDirty().position;
	}
	INLINE glm::quat& modifyRotation() {
		auto& rot = _markDirty().rotation;
		rot = glm::normalize(rot);
		return rot;
	}
	INLINE glm::vec3& modifyScale() {
		return _markDirty().scale;
	}
	INLINE StateField& modifyState() {
		return _markDirty().state;
	}
	INLINE glm::quat& setFromEuler(const glm::vec3& anglesDeg) {
		return modifyRotation() = glm::quat{ glm::radians(anglesDeg) };
	}

	INLINE const glm::vec3& position() const { return m_reg->get<TransformComponent>(m_entity).position; }
	INLINE const glm::vec3& scale() const { return m_reg->get<TransformComponent>(m_entity).scale; }
	INLINE const glm::quat& rotation() const { return m_reg->get<TransformComponent>(m_entity).rotation; }
	INLINE const StateField& state() const { return m_reg->get<TransformComponent>(m_entity).state; }
	INLINE const glm::vec3 eulerAngles() const { return glm::eulerAngles(m_reg->get<TransformComponent>(m_entity).rotation); }

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

	INLINE Transform& lerpBetween(const Transform& a, const Transform& b, float t) {
		auto& comp = _markDirty();
		const TransformComponent& A = a;
		const TransformComponent& B = b;
		comp.position = glm::mix(A.position, B.position, t);
		comp.rotation = glm::slerp(A.rotation, B.rotation, t);
		comp.scale = glm::mix(A.scale, B.scale, t);
		return *this;
	}

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

	INLINE void translate(const glm::vec3& direction) {
		auto& comp = _markDirty();
		comp.position += direction;
	}
	INLINE void rotate(const glm::vec3& rotationDelta) {
		auto& comp = _markDirty();
		comp.rotation = glm::normalize(glm::quat(rotationDelta) * comp.rotation);
	}
	INLINE void rotateLocal(const glm::vec3& rotDelta) {
		auto& comp = _markDirty();

		float yaw = rotDelta.x;
		float pitch = rotDelta.y;
		float roll = rotDelta.z;

		comp.rotation = glm::normalize(comp.rotation);

		// Get current pitch for clamping
		glm::vec3 forward = comp.rotation * DIR_FORWARD;
		float currentPitch = asin(glm::clamp(forward.y, -1.0f, 1.0f));

		// Clamp pitch
		const float maxPitch = glm::radians(89.0f);
		float newPitch = glm::clamp(currentPitch + pitch, -maxPitch, maxPitch);
		pitch = newPitch - currentPitch;

		// Apply rotations
		glm::quat yawRotation = glm::angleAxis(yaw, DIR_UP);
		glm::vec3 rightVector = comp.rotation * DIR_RIGHT;
		glm::quat pitchRotation = glm::angleAxis(pitch, rightVector);

		// Only apply roll if explicitly requested
		if (abs(roll) > 1e-6f) {
			glm::vec3 forwardVector = comp.rotation * DIR_FORWARD;
			glm::quat rollRotation = glm::angleAxis(roll, forwardVector);
			comp.rotation = glm::normalize(rollRotation * pitchRotation * yawRotation * comp.rotation);
		} else {
			comp.rotation = glm::normalize(yawRotation * pitchRotation * comp.rotation);
		}
	}

	INLINE void rotateAroundWorldAxis(const glm::vec3& axis, float angle) {
		auto& comp = _markDirty();
		glm::quat deltaRot = glm::angleAxis(angle, axis);
		comp.rotation = glm::normalize(deltaRot * comp.rotation);
	}
	INLINE void lookAt(const glm::vec3& target, const glm::vec3& up = DIR_UP) {
		glm::vec3 forward = glm::normalize(target - position());
		auto& rot = modifyRotation();
		rot = glm::quatLookAt(forward, up);
	}

	INLINE glm::vec3 transformPoint(const glm::vec3& localPoint) const {
		glm::vec4 worldPoint = localToWorld() * glm::vec4(localPoint, 1.0f);
		return glm::vec3(worldPoint);
	}
	INLINE glm::vec3 transformDirection(const glm::vec3& localDirection) const {
		auto& comp = m_reg->get<TransformComponent>(m_entity);
		return comp.rotation * (localDirection * comp.scale);
	}
	INLINE glm::vec3 transformNormal(const glm::vec3& localNormal) const {
		return normalMatrix() * localNormal;
	}



	std::span<entt::entity> getChildren();
	std::optional<Transform> getParent();

	void setParent(const Transform* parent);
	void setParent(const entt::entity& parent);
	void clearChildren();

	// Static
	INLINE static glm::vec3& position(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).position;
	}
	INLINE static glm::vec3& scale(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).scale;
	}
	INLINE static glm::quat& rotation(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).rotation;
	}
	INLINE static StateField& state(ArenaRegistry* registry, entt::entity entity) {
		return registry->get<TransformComponent>(entity).state;
	}

	static std::span<entt::entity> getChildren(ArenaRegistry* registry, entt::entity ofEntity);
	static std::optional<Transform> getParent(ArenaRegistry* registry, entt::entity ofEntity);
};

#endif