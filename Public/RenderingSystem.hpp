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
	std::vector<ArenaVector<DrawCommand>> s_pendingDrawCommands;
	std::vector<ArenaVector<DrawCommand>> s_threadPresistentDrawBuffers;
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

	uint16_t m_sceneIndex;

public:
	~SceneRenderSystem() {}
	SceneRenderSystem(const SceneRenderSystem&) = delete;
	SceneRenderSystem& operator=(const SceneRenderSystem&) = delete;
	SceneRenderSystem(SceneRenderSystem&&) = delete;
	SceneRenderSystem& operator=(SceneRenderSystem&&) = delete;
	SceneRenderSystem(ArenaRegistry* const registry, Arena* const arena, uint16_t sceneIndex) :
		SystemECS{ registry },
		m_vertexUpdates{ ArenaAllocator<std::pair<const VkBuffer, VertexUpdate>>(arena) },
		m_subMeshes{ ArenaAllocator<SubMesh>{arena} },
		m_vertices{ ArenaAllocator<BaseVSIn>{arena} },
		m_meshes{ ArenaAllocator<MeshData>{arena} },
		m_indices{ ArenaAllocator<uint32_t>{arena} },
		m_pendingDrawCommands{ ArenaAllocator<DrawCommand>{arena} },
		m_persistentDrawCommands{ ArenaAllocator<DrawCommand>{arena} },
		m_persistentDcmdHashIndexMap{ ArenaAllocator<std::pair<const uint64_t, size_t>>(arena) },
		m_sceneIndex{sceneIndex},
		s_pendingDrawCommands{ SCENE_MAX_SCENES, ArenaVector<DrawCommand>{RENDER_SCENE_MAX_THREADS, ArenaAllocator<DrawCommand>{arena} } },
		s_threadPresistentDrawBuffers{ SCENE_MAX_SCENES, ArenaVector<DrawCommand>{RENDER_SCENE_MAX_THREADS, ArenaAllocator<DrawCommand>{arena} } },
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

	uint64_t submitDraw(DrawCommand drawCmd) {

		setFrameThreadIndex();

		auto hash = drawCmd.hash();

		if (drawCmd.persistent) {
			
			auto it = s_persistentDcmdHashIndexMap[m_sceneIndex].find(hash);
			if (it == s_persistentDcmdHashIndexMap[m_sceneIndex].end()) {
				s_persistentDcmdHashIndexMap[m_sceneIndex].insert({ hash, s_threadPresistentDrawBuffers[m_sceneIndex].size() });
			}
		} else
			s_pendingDrawCommands[m_sceneIndex].push_back(drawCmd);
		
		return hash;
	}

	ErrorCode loadModel(const std::string& filename, uint32_t* outMeshIndex) {
		auto ec = AssetLoader::loadModel(filename,
									  m_meshes, m_vertices, m_subMeshes, m_indices,
									  *RenderManager::getInstance(), outMeshIndex);

		if (!EC_FAILED(ec)) {
			//to gpu buffers
		}

		return ec;
	}

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

	std::span<BaseVSIn> getVertices() { return std::span<BaseVSIn>(m_vertices); }
	std::span<SubMesh> getSubMeshes() { return std::span<SubMesh>(m_subMeshes); }
};

#endif