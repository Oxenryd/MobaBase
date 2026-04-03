#include "Mesh.hpp"
#include "Engine.h"
#include "Scene.h"

BoundingVolume Mesh::getBoundingVolume() {
	return BoundingVolume{m_reg, m_entity};
}





//void Mesh::setSubMeshFlags(uint8_t flags, uint32_t firstIndex, uint32_t lastIndex) {
//	
//	auto subs = getSubmeshes();
//	if (firstIndex >= subs.size())
//		return;
//	auto _last = (lastIndex == UINT32_INVALID)
//		? subs.size() - 1
//		: (lastIndex < firstIndex)
//		? firstIndex
//		: lastIndex;
//
//	auto _first = firstIndex == UINT32_INVALID ? 0 : firstIndex;
//
//	for (size_t i = _first; i < _last + 1; ++i) {
//
//	}
//}

Transform Mesh::getTransform() {
	return Transform(m_reg, m_entity);
}

MeshFilterComponent& Mesh::getMeshFilterComponent() {
	//const auto& [meshComp, trans] = m_reg->get<MeshComponent, TransformComponent>(m_entity);
	return m_reg->get<MeshFilterComponent>(m_entity);
	//return Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getMeshes()[meshComp.meshIndex];
}

// std::span<MeshData> Mesh::getSubmeshes() {
// 	const auto& [meshComp, trans] = m_reg->get<MeshFilterComponent, TransformComponent>(m_entity);
// 	//auto& meshData = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getMeshes()[meshComp.meshIndex];
// 	auto& subMeshes = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getSubMeshes();
//
// 	return std::span<MeshData>(&subMeshes[meshComp.subMeshOffset], meshComp.subMeshCount);
// }


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

	const auto& [meshComp, trans] = m_reg->get<MeshFilterComponent, TransformComponent>(m_entity);
	const auto& subMeshes = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getSubMeshes();
	auto& vertices = Engine::getInstance()->getScene(trans.sceneIndex)->sceneRender().getVertices();
	auto& mData = subMeshes[meshComp.meshDataIndex];
	//auto& lastSub = subMeshes[meshComp.subMeshOffset + meshComp.subMeshCount - 1];

	//auto count = (lastSub.vertexOffset + lastSub.vertexCount) - firstSub.vertexOffset;
	return std::span<BaseVSIn>(&vertices[mData.vertexOffset], mData.vertexCount);
}

