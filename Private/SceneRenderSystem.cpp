#include "SceneRenderSystem.hpp"
#include "Engine.h"
#include "MobaMath.hpp"

#include <format>






ErrorCode SceneRenderSystem::loadModel(const std::string& filename, MeshDescription* outMeshInfo) {
	
	uint32_t startTexIndex = RenderManager::getInstance()->textures().size();

	MeshDescription mInfo{};
	auto ec = AssetLoader::loadModel(filename,
									 m_meshes, m_vertices, m_subMeshes, m_indices,
									 *RenderManager::getInstance(), &mInfo);

	uint32_t endTexIndex = RenderManager::getInstance()->textures().size();

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
		RenderManager::getInstance()->vkContext()->reallocateVertexIndexBuffers();

		for (size_t i = startTexIndex; i < endTexIndex; ++i) {
			auto& tex = RenderManager::getInstance()->textures()[i];
			tex.tryAllocate();
		}
		RenderManager::getInstance()->vkContext()->loadBaseMatData();

		if (outMeshInfo) {
			*outMeshInfo = mInfo;
		}
	}

	return ec;
}

uint32_t SceneRenderSystem::addCamera(CameraData* initData) {
	CameraData cData = initData ? *initData : CameraData{};

	auto& scene = *Engine::getInstance()->getScene(m_sceneIndex);
	auto& cams = scene.gameObjectSystem().getAllOfType<Camera>();
	auto index = cams.size();
	std::string name = std::format("Camera_{}.{}", index, m_sceneIndex);

	auto& newCam = scene.gameObjectSystem().createGameObject<Camera>(name);
	
	auto& comp = m_reg->get<TransformComponent>(newCam.m_entity);
	auto camIndex = CamIndex{ m_sceneIndex, static_cast<uint32_t>(index) };
	comp.callbackUserData = reinterpret_cast<void*>(camIndex._raw);
	comp.onDirtyCallback = [](void* ctx) -> void {
		CamIndex camIndex = CamIndex{ reinterpret_cast<uint64_t>(ctx) };
		auto& this_ = Engine::getInstance()->getScene(camIndex.sceneIndex)->gameObjectSystem()
				.getAllOfType<Camera>()[camIndex.camIndex];

		this_.m_viewDirty = true;
		};

	return index;
}

ErrorCode SceneRenderSystem::createMeshFromModel(const std::string& path, Mesh* outMesh, const GameObject* go) {

	MeshDescription meshInfo{};
	auto ec = loadModel(path, &meshInfo);
	if (!EC_FAILED(ec)) {
		Mesh newMesh{};
		MeshComponent meshComp{};
		meshComp.meshIndex = meshInfo.meshIndex;
		if (go) {
			meshComp = m_reg->emplace<MeshComponent>(go->entity(), meshComp);
			newMesh = Mesh{ m_reg, go->entity() };
			if (outMesh) {
				*outMesh = newMesh;
			}
		} else {
			auto newEntity = m_reg->create();
			meshComp = m_reg->emplace<MeshComponent>(newEntity, meshComp);
			newMesh = Mesh{ m_reg, newEntity };
			if (outMesh) {
				*outMesh = newMesh;
			}
		}

		for (size_t i = meshInfo.subMeshOffset; i < meshInfo.subMeshOffset + meshInfo.subMeshCount; ++i) {
			auto& subMesh = m_subMeshes[i];
			subMesh.entity = m_reg->create();
			m_reg->emplace<SubMeshComponent>(subMesh.entity, SubMeshComponent{ static_cast<uint32_t>(i) });
			
			auto subVerts = std::span<BaseVSIn>(&m_vertices[subMesh.vertexOffset], subMesh.vertexCount);

			Engine::getInstance()->getScene(m_sceneIndex)->transformSystem().registryEmplace(subMesh.entity);

			AABB box{};
			box.encloseLocal(subVerts);
			BoundingVolumeComponent boundComp{};
			Engine::getInstance()->getScene(m_sceneIndex)->boundingSystem().registryEmplace(subMesh.entity, &box, &boundComp);
		}
		
	} else
		return ec;

	return ErrorCode::OK;
}
