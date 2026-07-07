#ifndef DRAWCOMMAND_HPP
#define DRAWCOMMAND_HPP

#include "ArenaAllocator.hpp"

class MaterialInstance;

enum class DrawType : uint8_t
{
	Mesh,
	SkinnedMesh,
	Billboard,
	Sprite,
	UI,
	Custom
};

struct DrawCommand
{
	MaterialInstance* material;
	void* drawContextPtr;
	uint16_t priority;
	DrawType type;
	bool instanceRequest;
	bool persistent;

	uint64_t hash() {
		uint64_t seed = 0;
		seed ^= std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(material) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		seed ^= std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(drawContextPtr) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		seed ^= std::hash<uint64_t>{}(priority) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(instanceRequest) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};


struct MeshDrawCommand
{
	uint32_t submeshOffset;
	uint32_t materialIndex;
	entt::entity subMeshEntity;
	uint32_t instanceIndex{UINT32_INVALID};
	uint16_t sceneIndex;
	uint16_t priority;
	

	
	uint64_t hash() {
		uint64_t seed = 0;
		seed ^= std::hash<uint64_t>{}(submeshOffset + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		seed ^= std::hash<uint64_t>{}(static_cast<uint32_t>(subMeshEntity) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		seed ^= std::hash<uint64_t>{}(materialIndex)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(instanceIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(sceneIndex)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}

	bool operator<(const MeshDrawCommand& other) const {
		return std::tie(materialIndex, priority) < std::tie(other.materialIndex, other.priority);
	}
};

#endif