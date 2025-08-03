#include "RenderingSystem.hpp"
#include "VulkanContext.hpp"

ErrorCode SceneRenderSystem::loadModel(const std::string& filename, MeshDescription* outMeshInfo) {
	
	MeshDescription mInfo{};

	auto ec = AssetLoader::loadModel(filename,
									 m_meshes, m_vertices, m_subMeshes, m_indices,
									 *RenderManager::getInstance(), &mInfo);

	if (!EC_FAILED(ec)) {
		
		for (size_t i = 0; i < mInfo.subMeshCount; ++i) {

			auto& subMesh = m_subMeshes[mInfo.subMeshOffset + i];

			auto vOffset = subMesh.vertexOffset;
			auto vCount = subMesh.vertexCount;
			auto iOffset = subMesh.indexOffset;
			auto iCount = subMesh.indexCount;

			RenderManager::getInstance()->vkContext()->registerMesh(
				&m_vertices[vOffset], vCount,
				&m_indices[iOffset], iCount);

		}


	}

	return ec;
}