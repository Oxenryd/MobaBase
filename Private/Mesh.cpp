#include "MeshComponent.hpp"
#include "Engine.h"

std::span<BaseVSIn> SubMesh::getVertices() {
	return std::span<BaseVSIn>();
}





MeshComponent& Mesh::getMeshComponent() {
	//const auto& [meshComp, trans] = m_reg->get<MeshComponent, TransformComponent>(m_entity);
	return m_reg->get<MeshComponent>(m_entity);
	//return Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getMeshes()[meshComp.meshIndex];
}

std::span<SubMeshData> Mesh::getSubmeshes() {
	const auto& [meshComp, trans] = m_reg->get<MeshComponent, TransformComponent>(m_entity);
	//auto& meshData = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getMeshes()[meshComp.meshIndex];
	auto& subMeshes = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getSubMeshes();

	return std::span<SubMeshData>(&subMeshes[meshComp.subMeshOffset], meshComp.subMeshCount);
}


glm::vec3 Mesh::getAvgCenter() {

	auto vertices = getVertices();

	if (vertices.empty())
		return glm::vec3{ 0 };

	glm::vec3 total{ 0 };
	for (auto& vert : vertices)
		total += vert.pos;

	return total / static_cast<float>(vertices.size());
}

std::span<BaseVSIn> Mesh::getVertices() {

	const auto& [meshComp, trans] = m_reg->get<MeshComponent, TransformComponent>(m_entity);
	//auto& meshData = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getMeshes()[meshComp.meshIndex];
	auto& subMeshes = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getSubMeshes();
	auto& vertices = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getVertices();

	auto& firstSub = subMeshes[meshComp.subMeshOffset];
	auto& lastSub = subMeshes[meshComp.subMeshOffset + meshComp.subMeshCount - 1];

	auto count = (lastSub.vertexOffset + lastSub.vertexCount) - firstSub.vertexOffset;
	return std::span<BaseVSIn>(&vertices[firstSub.vertexOffset], count);
}

