#include "SceneRenderSystem.hpp"
#include "Engine.h"
#include "MMath.hpp"

#include <format>






ErrorCode SceneRenderSystem::loadModel(const std::string& filename, MeshComponent* outMeshInfo) {
	ErrorCode ec{};
	uint32_t startTexIndex = static_cast<uint32_t>(RenderManager::getInstance()->textures().size());
	MeshComponent mInfo{};
	auto it = m_pathMeshMap.find(filename);
	if (it != m_pathMeshMap.end()) {
		mInfo = it->second;

	} else {
		ec = AssetLoader::loadModel(filename,
										 m_vertices, m_subMeshes, m_indices,
										 *RenderManager::getInstance(), &mInfo);

		uint32_t endTexIndex = static_cast<uint32_t>(RenderManager::getInstance()->textures().size());

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

			m_pathMeshMap.insert({filename, mInfo});
		}
	}

	if (outMeshInfo) {
		*outMeshInfo = mInfo;
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

	return static_cast<uint32_t>(index);
}

ErrorCode SceneRenderSystem::createMeshFromModel(const std::string& path, Mesh* outMesh, const entt::entity parent) {

	MeshComponent meshComp{};
	auto ec = loadModel(path, &meshComp);
	if (EC_FAILED(ec))
		return ec;

	entt::entity meshEntity;
	if (parent != entt::null) {
		auto trans = m_reg->try_get<TransformComponent>(parent);
		if (!trans)
			throw std::exception("Meshes needs transforms!!");

		meshEntity = parent;
		meshComp = m_reg->emplace<MeshComponent>(parent, meshComp);

	} else {

		meshEntity = m_reg->create();
		Engine::getInstance()->getScene(m_sceneIndex)->transformSystem().registryEmplace(
			meshEntity,
			nullptr
		);

		meshComp = m_reg->emplace<MeshComponent>(meshEntity, meshComp);
	}


	Mesh newMesh{m_reg, meshEntity };
	if (outMesh) {
		*outMesh = newMesh;
	}

	//float maxVol = 0.0f;
	//entt::entity biggestSubEntity = entt::null;
	for (size_t i = meshComp.subMeshOffset; i < meshComp.subMeshOffset + meshComp.subMeshCount; ++i) {

		entt::entity subEnt = m_reg->create();
		SubMeshData& subMesh = m_subMeshes[i];
		subMesh.entity = subEnt;

		//subMesh.entity = parent != entt::null ? parent : m_reg->create();
		//subMesh.parent = parent;
		//[[maybe_unused]] auto& newSubMeshComp = 

		m_reg->emplace<SubMeshComponent>(subEnt, SubMeshComponent{ meshEntity, static_cast<uint32_t>(i) });

		auto subVerts = std::span<BaseVSIn>(&m_vertices[subMesh.vertexOffset], subMesh.vertexCount);

		Engine::getInstance()->getScene(m_sceneIndex)->transformSystem().registryEmplace(
			subEnt,
			parent == entt::null ? nullptr : static_cast<void*>(&meshEntity)
		);

		AABB box{};
		box.encloseLocal(subVerts);
		Engine::getInstance()->getScene(m_sceneIndex)->boundingSystem().registryEmplace(subEnt, &box);

		//if (parent != entt::null) {
		//	auto newTransform = Transform{ m_reg, subMesh.entity };
		//	newTransform.setParent(parent);
		//}
	}
	return ErrorCode::OK;
}
