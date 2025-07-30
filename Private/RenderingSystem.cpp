#include "RenderingSystem.hpp"
#include "VulkanContext.hpp"

ErrorCode SceneRenderSystem::loadModel(const std::string& filename, MeshDescription* outMeshInfo) {
	
	MeshDescription mInfo{};

	auto ec = AssetLoader::loadModel(filename,
									 m_meshes, m_vertices, m_subMeshes, m_indices,
									 *RenderManager::getInstance(), &mInfo);

	if (!EC_FAILED(ec)) {
		
		RenderManager::getInstance()->vkContext()->registerMesh(
			&m_vertices[mInfo.vertexOffset], mInfo.vertexCount,
			&m_indices[mInfo.indexOffset], mInfo.indexCount);
	}

	return ec;
}