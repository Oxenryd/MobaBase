#include "SceneRenderSystem.hpp"
#include "Engine.h"
#include "MMath.hpp"

#include <format>






ErrorCode SceneRenderSystem::loadModel(
		const std::string& filename,
		MeshComponent* outMeshInfo,
		MeshNameVector* outMeshNames)
{
	ErrorCode ec{};
	const uint32_t startTexIndex = static_cast<uint32_t>(RenderManager::getInstance()->textures().size());
	MeshComponent* mInfo = nullptr;
	MeshNameVector* names = nullptr;
	const auto it = m_pathMeshMap.find(filename);
	if (it != m_pathMeshMap.end()) {
		mInfo = &it->second.first;
		names = &it->second.second;

	} else {

		auto [storedData, inserted] = m_pathMeshMap.insert(
			{filename, {MeshComponent{}, MeshNameVector{}} });
		if (!inserted)
			return ErrorCode::ASSETS_MODEL_LOAD_NEW_ALREADY_FOUND;

		mInfo = &storedData->second.first;
		names = &storedData->second.second;

		ec = AssetLoader::loadModel(filename,
										 m_vertices, m_subMeshes, m_indices,
										 *RenderManager::getInstance(), mInfo, names);

		const uint32_t endTexIndex = static_cast<uint32_t>(RenderManager::getInstance()->textures().size());

		if (!EC_FAILED(ec)) {

			for (size_t i = 0; i < mInfo->subMeshCount; ++i) {

				const auto& subMesh = m_subMeshes[mInfo->subMeshOffset + i];

				const auto vOffset = subMesh.vertexOffset;
				const auto vCount = subMesh.vertexCount;
				const auto iOffset = subMesh.indexOffset;
				const auto iCount = subMesh.indexCount;

				RenderManager::getInstance()->vkContext()->registerMesh(
					&m_vertices[vOffset], vCount,
					&m_indices[iOffset], iCount);

				if (names->at(i).empty()) {
					names->at(i) = "Submesh";
				}

			}
			RenderManager::getInstance()->vkContext()->reallocateVertexIndexBuffers();

			for (size_t i = startTexIndex; i < endTexIndex; ++i) {
				auto& tex = RenderManager::getInstance()->textures()[i];
				tex.tryAllocate();
			}
			RenderManager::getInstance()->vkContext()->loadBaseMatData();

			//m_pathMeshMap.insert({filename, {mInfo, meshNames}});
		}
	}

	if (outMeshInfo) {
		outMeshInfo = mInfo;
	}
	if (names) {
		outMeshNames = names;
	}

	return ec;
}

uint32_t SceneRenderSystem::addCamera(const CameraData* initData) const {
	CameraData cData = initData ? *initData : CameraData{};
	auto& scene = *Engine::getInstance()->getScene(m_sceneIndex);
	const auto& cams = scene.gameObjectSystem().getAllOfType<Camera>();
	auto index = cams.size();
	const std::string name = std::format("Camera_{}.{}", index, static_cast<uint32_t>(m_sceneIndex));

	const auto& newCam = scene.gameObjectSystem().createGameObject<Camera>(name, nullptr, cData);
	
	auto& comp = m_reg->get<TransformComponent>(newCam.m_entity);
	const auto camIndex = CamIndex{ m_sceneIndex, static_cast<uint32_t>(index) };
	comp.callbackUserData = reinterpret_cast<void*>(camIndex.camIndex);
	comp.onDirtyCallback = [](void* ctx) -> void {
		const auto camIdx = CamIndex{ reinterpret_cast<uint64_t>(ctx) };
		auto& this_ = Engine::getInstance()->getScene(camIdx.sceneIndex)->gameObjectSystem()
				.getAllOfType<Camera>()[camIdx.camIndex];

		this_.m_viewDirty = true;
		};

	return static_cast<uint32_t>(index);
}

ErrorCode SceneRenderSystem::createMeshFromModel(const std::string& path, Mesh* outMesh, const GameObject* parent) {

	MeshComponent* meshComp = nullptr;
	MeshNameVector* meshNames = nullptr;
	const auto ec = loadModel(path, meshComp, meshNames);
	if (EC_FAILED(ec))
		return ec;

	entt::entity meshEntity;
	if (parent) {
		const auto trans = m_reg->try_get<TransformComponent>(parent->entity());
		if (!trans)
			throw std::runtime_error("Meshes needs transforms!!");

		meshEntity = parent->entity();
		m_reg->emplace<MeshComponent>(parent->entity(), *meshComp);

	} else {


		// for now::
		return ErrorCode::TRANSFORM_MESH_ORPHANED;

		// meshEntity = m_reg->create();
		// Engine::getInstance()->getScene(m_sceneIndex)->transformSystem().registryEmplace(
		// 	meshEntity,
		// 	nullptr, nullptr
		// );
		//
		// meshComp = m_reg->emplace<MeshComponent>(meshEntity, meshComp);
	}


	const Mesh newMesh{m_reg, meshEntity };
	if (outMesh) {
		*outMesh = newMesh;
	}

	//float maxVol = 0.0f;
	//entt::entity biggestSubEntity = entt::null;
	size_t count = 0;
	for (size_t i = meshComp->subMeshOffset; i < meshComp->subMeshOffset + meshComp->subMeshCount; ++i) {

		auto subMeshGO = Engine::getInstance()->getScene(m_sceneIndex)->gameObjectSystem()
			.createGameObject<GameObject>(meshNames->at(count++), nullptr);


		const entt::entity subEnt = m_reg->create();
		SubMeshData& subMesh = m_subMeshes[i];
		subMesh.entity = subEnt;

		//subMesh.entity = parent != entt::null ? parent : m_reg->create();
		//subMesh.parent = parent;
		//[[maybe_unused]] auto& newSubMeshComp = 

		m_reg->emplace<SubMeshComponent>(subEnt, SubMeshComponent{ meshEntity, static_cast<uint32_t>(i) });

		const auto subVerts = std::span<BaseVSIn>(&m_vertices[subMesh.vertexOffset], subMesh.vertexCount);

		Engine::getInstance()->getScene(m_sceneIndex)->transformSystem().registryEmplace(
			subEnt,
			parent == entt::null ? nullptr : static_cast<void*>(&meshEntity), nullptr
		);

		AABB box{};
		box.encloseLocal(subVerts);
		Engine::getInstance()->getScene(m_sceneIndex)->boundingSystem().registryEmplace(subEnt, &box, nullptr);

		//if (parent != entt::null) {
		//	auto newTransform = Transform{ m_reg, subMesh.entity };
		//	newTransform.setParent(parent);
		//}
	}
	return ErrorCode::OK;
}
