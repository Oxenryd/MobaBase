#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>
#include "ArenaAllocator.hpp"


struct MeshComponent
{
	uint32_t meshIndex;
};

struct MeshData
{
	size_t firstSubMeshIndex;
	size_t subMeshCount;
};

struct SubMesh
{
	uint32_t vertexOffset;
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t materialIndex;
};

struct Mesh
{

private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	~Mesh() = default;
	Mesh() = delete;
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
	}
	bool operator==(const Mesh& rhs) {
		return m_reg == rhs.m_reg && m_entity == rhs.m_entity;
	}

	//... to be filled with accessors
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