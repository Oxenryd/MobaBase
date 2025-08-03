#ifndef RENDERING_SYSTEM_HPP
#define RENDERING_SYSTEM_HPP

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

public:
	~SceneRenderSystem() {}
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

	void submitPersistent(entt::entity entity, uint32_t meshIndex, uint16_t matInstancecIndex, uint16_t prio) {
		setFrameThreadIndex();

		auto& mesh = m_reg->get<MeshComponent>(entity);
		for (size_t i = 0; i < m_meshes[mesh.meshIndex].subMeshCount; ++i) {
			MeshDrawCommand dCmd{};
			dCmd.entityId = entity;
			dCmd.priority = prio;
			dCmd.instanceIndex = matInstancecIndex;
			dCmd.sceneIndex = m_sceneIndex;
			dCmd.submeshOffset = m_meshes[mesh.meshIndex].firstSubMeshIndex + i;
			dCmd.materialIndex = m_subMeshes[dCmd.submeshOffset].materialIndex;
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

	std::span<MeshDrawCommand> persistentDrawCommands() { 
		return std::span<MeshDrawCommand>(s_threadPresistentDrawBuffers[m_sceneIndex]); }

	std::span<BaseVSIn> getVertices() { return std::span<BaseVSIn>(m_vertices); }
	std::span<SubMesh> getSubMeshes() { return std::span<SubMesh>(m_subMeshes); }
};

#endif