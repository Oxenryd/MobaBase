#include "Transform.hpp"
#include "TransformSystem.hpp"
#include "Engine.h"

std::span<entt::entity> Transform::getChildren() {
	auto& comp = m_reg->get<TransformComponent>(m_entity);
	return Engine::getInstance()->getScene(comp.sceneIndex)->transformSystem().getChildren(m_entity);
	//return result ? std::optional<std::vector<entt::entity>&>{*result} : std::nullopt;
}

std::optional<Transform> Transform::getParent() {
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

std::span<entt::entity> Transform::getChildren(ArenaRegistry* registry, entt::entity ofEntity) {
	auto comp = registry->try_get<TransformComponent>(ofEntity);
	if (!comp)
		return std::span<entt::entity>();

	return Engine::getInstance()->getScene(comp->sceneIndex)->transformSystem().getChildren(ofEntity);
	//return result ? std::optional<std::vector<entt::entity>&>{*result} : std::nullopt;
}

std::optional<Transform> Transform::getParent(ArenaRegistry* registry, entt::entity ofEntity) {
	auto comp = registry->try_get<TransformComponent>(ofEntity);
	if (!comp)
		return std::nullopt;

	auto result = Engine::getInstance()->getScene(comp->sceneIndex)->transformSystem().getParent(ofEntity);
	return result != entt::null ? std::optional<Transform>{Transform{ registry, result }} : std::nullopt;
}
