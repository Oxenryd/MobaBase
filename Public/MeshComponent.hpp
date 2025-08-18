#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <span>

#include "ArenaAllocator.hpp"
#include "HlslTypes.h"
#include "Transform.hpp"
#include "BoundingVolume.hpp"

//struct MeshDescription
//{
//	uint32_t subMeshOffset;
//	uint32_t subMeshCount;
//};


struct SubMeshComponent
{
	uint32_t subMeshIndex{ UINT32_INVALID };
};

struct MeshComponent
{
	uint32_t subMeshOffset;
	uint32_t subMeshCount;
};

struct SubMeshData
{
	entt::entity parent{ entt::null };
	//entt::entity entity{ entt::null };
	uint32_t vertexOffset{ UINT32_INVALID };
	uint32_t vertexCount{ UINT32_INVALID };
	uint32_t indexOffset{ UINT32_INVALID };
	uint32_t indexCount{ UINT32_INVALID };
	uint32_t materialIndex{ UINT32_INVALID };
	uint32_t instanceIndex{ UINT32_INVALID };
};


struct SubMesh
{
private:
	ArenaRegistry* m_reg;
	entt::entity m_entity;

public:
	glm::vec3 getAvgCenter();
	std::span<BaseVSIn> getVertices();
	BoundingVolume getBoundingVolume();
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
	Transform getTransform();
	MeshComponent& getMeshComponent();
	glm::vec3 getAvgCenter();
	std::span<BaseVSIn> getVertices();
	std::span<SubMeshData> getSubmeshes();
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