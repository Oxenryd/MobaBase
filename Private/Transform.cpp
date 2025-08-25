#include "Transform.hpp"
#include "TransformSystem.hpp"
#include "Engine.h"

TransformComponent& Transform::_markDirty(uint8_t what) {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	switch (what) {
		default: break;
		case TRANSLATION:
			comp.state.setByEnum(ObjectState::TranslationDirty); break;
		case ROTATION:
			comp.state.setByEnum(ObjectState::RotationDirty); break;
		case SCALE:
			comp.state.setByEnum(ObjectState::ScaleDirty); break;
		case TRANSFORM:
		{
			comp.state.setByEnum(ObjectState::TranslationDirty);
			comp.state.setByEnum(ObjectState::RotationDirty);
			comp.state.setByEnum(ObjectState::ScaleDirty);
		} break;
	}
	comp.state.setByEnum(ObjectState::DirtyTransform);
	if (comp.onDirtyCallback) {
		comp.onDirtyCallback(comp.callbackUserData);
	}
	return comp;
}

glm::vec3& Transform::modifyPosition() {
	auto& comp = _markDirty(TRANSLATION);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex];
}

glm::vec3& Transform::modifyScale() {
	auto& comp = _markDirty(SCALE);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex];
}

glm::quat& Transform::modifyRotation() {
	auto& comp = _markDirty(ROTATION);
	//auto& rot = .rotation;
	//rot = glm::normalize(rot);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
}

const glm::vec3& Transform::position() const { 
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex];
}

const glm::vec3& Transform::scale() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex];
}

const glm::quat& Transform::rotation() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
}

const glm::mat4x4& Transform::localToWorld() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().modelTransforms().at(comp.dataIndex);
}

const glm::mat4x4 Transform::worldToLocal() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return glm::inverse(
		static_cast<glm::mat4x4>(
			Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().modelTransforms().at(comp.dataIndex)
			)
	); // MIGHT CACHE THIS IN THE FUTURE TOO INSIDE THE SYSTEM LIKE LOCALTOWORLD
}

Transform& Transform::lerpBetween(const Transform& a, const Transform& b, float t) {
	auto& comp = _markDirty(TRANSFORM);

	auto& position = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex];
	auto& rotation = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
	auto& scale = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex];

	position = glm::mix(a.position(), b.position(), t);
	rotation = glm::slerp(a.rotation(), b.rotation(), t);
	scale = glm::mix(a.scale(), b.scale(), t);
	return *this;
}

void Transform::translate(const glm::vec3& direction) {
	auto& comp = _markDirty(TRANSLATION);
	Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex] += direction;
}

void Transform::rotate(const glm::vec3& rotationDelta) {
	auto& comp = _markDirty(ROTATION);
	auto& rotation = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
	rotation =
		glm::normalize(glm::quat(rotationDelta) * rotation);
}

void Transform::rotateLocal(const glm::vec3& rotDelta) {
	auto& comp = _markDirty(ROTATION);

	float yaw = rotDelta.x;
	float pitch = rotDelta.y;
	float roll = rotDelta.z;

	auto& rotation = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];

	rotation = glm::normalize(rotation);

	// Get current pitch for clamping
	glm::vec3 forward = rotation * DIR_FORWARD;
	float currentPitch = asin(glm::clamp(forward.y, -1.0f, 1.0f));

	// Clamp pitch
	const float maxPitch = glm::radians(89.0f);
	float newPitch = glm::clamp(currentPitch + pitch, -maxPitch, maxPitch);
	pitch = newPitch - currentPitch;

	// Apply rotations
	glm::quat yawRotation = glm::angleAxis(yaw, DIR_UP);
	glm::vec3 rightVector = rotation * DIR_RIGHT;
	glm::quat pitchRotation = glm::angleAxis(pitch, rightVector);

	// Only apply roll if explicitly requested
	if (abs(roll) > 1e-6f) {
		glm::vec3 forwardVector = rotation * DIR_FORWARD;
		glm::quat rollRotation = glm::angleAxis(roll, forwardVector);
		rotation = glm::normalize(rollRotation * pitchRotation * yawRotation * rotation);
	} else {
		rotation = glm::normalize(yawRotation * pitchRotation * rotation);
	}
}

void Transform::rotateAroundWorldAxis(const glm::vec3& axis, float angle) {
	auto& comp = _markDirty(ROTATION);
	auto& rotation = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
	glm::quat deltaRot = glm::angleAxis(angle, axis);
	rotation = glm::normalize(deltaRot * rotation);
}

glm::vec3 Transform::transformDirection(const glm::vec3& localDirection) const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	auto& rotation = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
	auto& scale = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex];
	return rotation * (localDirection * scale);
}

std::string_view Transform::getTag() const {
	TransformComponent transform = *this;
	return Engine::getInstance()->getGlobalSystem().getTag({m_entity, transform.sceneIndex});
	//auto tagPtr = Engine::getInstance()->getGlobalSystem().registry().try_get<TagComponent>(m_entity);
	//if (tagPtr) {
	//	return std::string_view({tagPtr->tag.to_stringView()});
	//}
	return std::string_view();
}

std::span<entt::entity> Transform::getChildrenEntities() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().getChildren(m_entity);
}

std::optional<Transform> Transform::getParentTransform() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	auto result = Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().getParent(m_entity);
	return result != entt::null ? std::optional<Transform>{Transform{m_reg, result}} : std::nullopt;
}

void Transform::setParent(const Transform* parent) {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	const entt::entity* entityParentPtr = parent != nullptr ? &parent->m_entity : nullptr;
	Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().setParent(m_entity, entityParentPtr);
}

void Transform::setParent(const entt::entity& parent) {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	const entt::entity* entityParentPtr = parent != entt::null ? &parent : nullptr;
	Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().setParent(m_entity, entityParentPtr);
}

void Transform::clearChildren() {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().clearChildren(m_entity);
}

glm::vec3& Transform::position(ArenaRegistry* registry, entt::entity entity) {
	auto& comp = registry->get<TransformComponent>(entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex];
}

glm::vec3& Transform::scale(ArenaRegistry* registry, entt::entity entity) {
	auto& comp = registry->get<TransformComponent>(entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex];
}

glm::quat& Transform::rotation(ArenaRegistry* registry, entt::entity entity) {
	auto& comp = registry->get<TransformComponent>(entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex];
}

glm::mat4x4 Transform::composeTRS(ArenaRegistry* registry, entt::entity entity) {
	auto& comp = registry->get<TransformComponent>(entity);
	return MMath::composeTRS(
		Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().positions()[comp.dataIndex],
		Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().rotations()[comp.dataIndex],
		Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().scales()[comp.dataIndex]);
}

std::span<entt::entity> Transform::getChildrenEntities(ArenaRegistry* registry, entt::entity ofEntity) {
	auto comp = registry->try_get<TransformComponent>(ofEntity);
	if (!comp)
		return std::span<entt::entity>();

	return Engine::getInstance()->getScene(comp->sceneIndex)->transformSystem().getChildren(ofEntity);
}

std::optional<Transform> Transform::getParentTransform(ArenaRegistry* registry, entt::entity ofEntity) {
	auto comp = registry->try_get<TransformComponent>(ofEntity);
	if (!comp)
		return std::nullopt;

	auto result = Engine::getInstance()->getScene(comp->sceneIndex)->transformSystem().getParent(ofEntity);
	return result != entt::null ? std::optional<Transform>{Transform{ registry, result }} : std::nullopt;
}
