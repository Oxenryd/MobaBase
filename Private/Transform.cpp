#include "Transform.hpp"
#include "TransformSystem.hpp"
#include "Engine.h"

const glm::mat4x4& Transform::localToWorld() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().modelTransforms().at(comp.matrixIndex);
}

const glm::mat4x4 Transform::worldToLocal() const {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return glm::inverse(
		static_cast<glm::mat4x4>(
			Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().modelTransforms().at(comp.matrixIndex)
			)
	); // MIGHT CACHE THIS IN THE FUTURE TOO INSIDE THE SYSTEM LIKE LOCALTOWORLD
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
