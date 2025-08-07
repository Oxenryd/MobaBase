#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <span>

#include "ArenaAllocator.hpp"
#include "HlslTypes.h"
#include "Transform.hpp"

struct MeshDescription
{
	uint32_t meshIndex;
	uint32_t subMeshOffset;
	uint32_t subMeshCount;
};

struct MeshComponent
{
	uint32_t meshIndex;
};

struct SubMeshComponent
{
	uint32_t subMeshIndex;
};

struct MeshData
{
	size_t firstSubMeshIndex;
	size_t subMeshCount;
};

struct SubMesh
{
	entt::entity entity{ entt::null };
	uint32_t vertexOffset{ UINT32_INVALID };
	uint32_t vertexCount{ UINT32_INVALID };
	uint32_t indexOffset{ UINT32_INVALID };
	uint32_t indexCount{ UINT32_INVALID };
	uint32_t materialIndex{ UINT32_INVALID };
	uint32_t instanceIndex{ UINT32_INVALID };

	glm::vec3 getAvgCenter();
	std::span<BaseVSIn> getVertices();
};


struct Mesh
{

private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	~Mesh() = default;
	Mesh() :
		m_reg{ nullptr }, m_entity{entt::null} {}
	Mesh(ArenaRegistry* registry, entt::entity entity)
		: m_reg{ registry }, m_entity{ entity } {}
	Mesh(const Mesh& other) {
		m_reg = other.m_reg;
		m_entity = other.m_entity;
	}
	Mesh& operator=(const Mesh& rhs) {
		if (this == &rhs)
			return *this;
		m_reg = rhs.m_reg;
		m_entity = rhs.m_entity;
		return *this;
	}
	Mesh(Mesh&& other) noexcept {
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;
	}
	Mesh& operator=(Mesh&& other) noexcept {
		
		m_reg = other.m_reg;
		m_entity = other.m_entity;

		other.m_reg = nullptr;
		other.m_entity = entt::null;

		return *this;
	}
	bool operator==(const Mesh& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}

	MeshData& getMeshData();
	glm::vec3 getAvgCenter();
	std::span<BaseVSIn> getVertices();
	std::span<SubMesh> getSubmeshes();
	//void rigSubMeshTransforms();
};


namespace entt
{
	template<>
	struct storage_type<MeshComponent, ArenaRegistry>
	{
		using type = ArenaStorage<MeshComponent>;
	};
}

#endif