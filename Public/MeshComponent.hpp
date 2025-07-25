#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>

struct MeshComponent
{
	uint32_t meshIndex;
};

struct Mesh
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


#endif