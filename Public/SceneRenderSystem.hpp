#ifndef SCENERENDER_SYSTEM_HPP
#define SCENERENDER_SYSTEM_HPP

#include <SystemECS.h>
#include <entt/entt.hpp>
#include <vulkan/vulkan_core.h>
#include <span>
#include <atomic>

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"
#include "MeshComponent.hpp"
#include "DrawCommand.hpp"
#include "AssetLoader.h"
#include "Camera.hpp"


#include "RenderManager.h" // WARNING!!


struct VertexUpdate
{
	size_t offset;
	size_t size;
	const void* data;
};

//RENDER_SCENE_MAX_THREADS

class SceneRenderSystem : public SystemECS
{
private:

	// Array of per-thread command lists
	//std::vector<ArenaVector<DrawCommand>> s_pendingDrawCommands;
	std::vector<ArenaVector<MeshDrawCommand>> s_threadPresistentDrawBuffers;
	std::vector<ArenaUMap<uint64_t, size_t>> s_persistentDcmdHashIndexMap;
	std::atomic<size_t> m_threadCounter = 0;
	static thread_local std::unordered_map<uint16_t, size_t> s_threadIndex;


	ArenaVector<DrawCommand> m_pendingDrawCommands;
	ArenaVector<DrawCommand> m_persistentDrawCommands;
	ArenaUMap<uint64_t, size_t> m_persistentDcmdHashIndexMap;

	ArenaUMap<VkBuffer, std::vector<VertexUpdate>> m_vertexUpdates;
	ArenaVector<MeshData> m_meshes;
	ArenaVector<SubMesh> m_subMeshes;
	ArenaVector<BaseVSIn> m_vertices;
	ArenaVector<uint32_t> m_indices;
	//ArenaVector<Camera> m_cameras;

public:
	~SceneRenderSystem() {
		s_threadIndex.clear();
		s_threadIndex.~unordered_map();
		new (&s_threadIndex) std::unordered_map<uint16_t, size_t>();
	}
	SceneRenderSystem(const SceneRenderSystem&) = delete;
	SceneRenderSystem& operator=(const SceneRenderSystem&) = delete;
	SceneRenderSystem(SceneRenderSystem&&) = delete;
	SceneRenderSystem& operator=(SceneRenderSystem&&) = delete;
	SceneRenderSystem(ArenaRegistry* const registry, Arena* const arena, uint16_t sceneIndex) :
		SystemECS{ registry, sceneIndex },
		m_vertexUpdates{ ArenaAllocator<std::pair<const VkBuffer, VertexUpdate>>(arena) },
		m_subMeshes{ ArenaAllocator<SubMesh>{arena} },
		m_vertices{ ArenaAllocator<BaseVSIn>{arena} },
		m_meshes{ ArenaAllocator<MeshData>{arena} },
		m_indices{ ArenaAllocator<uint32_t>{arena} },
		m_pendingDrawCommands{ ArenaAllocator<DrawCommand>{arena} },
		m_persistentDrawCommands{ ArenaAllocator<DrawCommand>{arena} },
		m_persistentDcmdHashIndexMap{ ArenaAllocator<std::pair<const uint64_t, size_t>>(arena) },
		//m_cameras { ArenaAllocator<Camera>{arena} },
		//s_pendingDrawCommands{ SCENE_MAX_SCENES, ArenaVector<DrawCommand>{RENDER_SCENE_MAX_THREADS, ArenaAllocator<DrawCommand>{arena} } },
		s_threadPresistentDrawBuffers{ SCENE_MAX_SCENES, ArenaVector<MeshDrawCommand>{ArenaAllocator<MeshDrawCommand>{arena} } },
		s_persistentDcmdHashIndexMap{ SCENE_MAX_SCENES, ArenaUMap<uint64_t, size_t>{RENDER_SCENE_MAX_THREADS, ArenaAllocator<std::pair<const uint64_t, size_t>>(arena) } }
	{
		s_threadIndex[m_sceneIndex] = SIZE_INVALID;

		//s_threadDrawBuffers = new ArenaVector<DrawCommand>[RENDER_SCENE_MAX_THREADS] {
		//	ArenaVector<DrawCommand>{ArenaAllocator<DrawCommand>{arena}}
		//	};
		//s_threadPresistentDrawBuffers = new ArenaVector<DrawCommand>[RENDER_SCENE_MAX_THREADS] {
		//	ArenaVector<DrawCommand>{ArenaAllocator<DrawCommand>{arena}}
		//	};
		//s_persistentDcmdHashIndexMap = new ArenaUMap<uint64_t, size_t>[RENDER_SCENE_MAX_THREADS] {
		//	ArenaUMap<uint64_t, size_t>{ArenaAllocator<std::pair<const uint64_t, size_t>>(arena)}
		//	};
	}

	INLINE void setFrameThreadIndex() {
		if (s_threadIndex[m_sceneIndex] == SIZE_INVALID)
			s_threadIndex[m_sceneIndex] = m_threadCounter++;
	}

	INLINE void submitPersistent(entt::entity entity, uint32_t meshIndex, uint16_t prio) {
		setFrameThreadIndex();

		auto& meshComp = m_reg->get<MeshComponent>(entity);
		auto& mesh = m_meshes[meshComp.meshIndex];
		for (size_t i = 0; i < mesh.subMeshCount; ++i) {
			auto& subMesh = m_subMeshes[mesh.firstSubMeshIndex + i];
			MeshDrawCommand dCmd{};
			dCmd.entityId = entity;
			dCmd.subMeshEntity = subMesh.entity;
			dCmd.priority = prio;
			dCmd.instanceIndex = subMesh.instanceIndex;
			dCmd.sceneIndex = m_sceneIndex;
			dCmd.submeshOffset = mesh.firstSubMeshIndex + i;
			dCmd.materialIndex = subMesh.materialIndex;
			s_threadPresistentDrawBuffers[m_sceneIndex].push_back(dCmd);
		}
	}

	uint64_t submitPersistentMeshDraw(MeshDrawCommand drawCmd) {

		setFrameThreadIndex();

		auto hash = drawCmd.hash();


		auto it = s_persistentDcmdHashIndexMap[m_sceneIndex].find(hash);
		if (it == s_persistentDcmdHashIndexMap[m_sceneIndex].end()) {
			s_persistentDcmdHashIndexMap[m_sceneIndex].insert({ hash, s_threadPresistentDrawBuffers[m_sceneIndex].size() });
		}
		

		return hash;
	}

	ErrorCode loadModel(const std::string& filename, MeshDescription* outMeshInfo);

	void afterDraw() {
		m_pendingDrawCommands.clear();
		m_threadCounter = 0;
		//s_threadIndex = SIZE_INVALID;
	}

	bool cancelPersistentDraw(uint64_t drawCmdHash) {
		auto it = s_persistentDcmdHashIndexMap[m_sceneIndex].find(drawCmdHash);
		if (it != s_persistentDcmdHashIndexMap[m_sceneIndex].end()) {
			s_persistentDcmdHashIndexMap[m_sceneIndex].erase(it->first);
			return true;
		}
		return false;
	}
	bool cancelPersistentDraw(DrawCommand drawCmd) {
		cancelPersistentDraw(drawCmd.hash());
	}

	uint32_t addCamera(CameraData* initData = nullptr);

	std::span<MeshDrawCommand> persistentDrawCommands() { 
		return std::span<MeshDrawCommand>(s_threadPresistentDrawBuffers[m_sceneIndex]); }

	ArenaVector<MeshData> getMeshes() { return m_meshes; }
	ArenaVector<BaseVSIn>& getVertices() { return m_vertices; }
	ArenaVector<SubMesh>& getSubMeshes() { return m_subMeshes; }
	//std::span<Camera> getCameras() { return std::span<Camera>(m_cameras); }

	ErrorCode createMeshFromModel(const std::string& path, Mesh* outMesh, const GameObject* go = nullptr);
};

#endif