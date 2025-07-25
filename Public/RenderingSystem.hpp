#ifndef RENDERING_SYSTEM_HPP
#define RENDERING_SYSTEM_HPP

#include <SystemECS.h>
#include <entt/entt.hpp>
#include <vulkan/vulkan_core.h>
#include <span>

#include "HlslTypes.h"
#include "ArenaAllocator.hpp"
#include "MeshComponent.hpp"

struct VertexUpdate
{
	size_t offset;
	size_t size;
	const void* data;
};


class RenderingSystem : public SystemECS
{
private:
	ArenaUMap<VkBuffer, std::vector<VertexUpdate>> m_vertexUpdates;
	ArenaVector<Mesh> m_meshes;
	ArenaVector<SubMesh> m_subMeshes;
	ArenaVector<BaseVSIn> m_vertices;
	ArenaVector<uint32_t> m_indices;

public:
	RenderingSystem(ArenaRegistry* const registry, Arena* const arena) :
		SystemECS{ registry },
		m_vertexUpdates{ ArenaAllocator<std::pair<const VkBuffer, VertexUpdate>>(arena) },
		m_subMeshes{ ArenaAllocator<SubMesh>{arena} },
		m_vertices{ ArenaAllocator<BaseVSIn>{arena} }
	{

	}

	std::span<BaseVSIn> getVertices() { return std::span<BaseVSIn>(m_vertices); }
	std::span<SubMesh> getSubMeshes() { return std::span<SubMesh>(m_subMeshes); }
};

#endif