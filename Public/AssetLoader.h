#ifndef ASSETLOADER_H
#define ASSETLOADER_H

#include <string>
#include <format>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "ArenaAllocator.hpp"
#include "GlobalMacros.h"
#include "Log.hpp"
#include "MeshComponent.hpp"
#include "ErrorCodes.hpp"
#include "HlslTypes.h"


INLINE glm::vec3 toVec3(const aiVector3D& aiVec) {
	return { aiVec.x, aiVec.y, aiVec.z };
}

INLINE glm::vec2 toVec2(const aiVector3D& aiVec) {
	return { aiVec.x, aiVec.y };
}

enum class AssetLoaderType : uint8_t
{
	OBJ_MTL
};


class AssetLoader
{
private:
	INLINE static ErrorCode _logReturnError(ErrorCode error, std::string msg) {
		LOGLINE(LogType::Error, LogMod::Assets, msg);
		return error;
	}


public:
	static ErrorCode loadModel(
		const std::string& filename,
		ArenaVector<Mesh>& meshes,
		ArenaVector<BaseVSIn>& vertexBuffer,
		ArenaVector<SubMesh>& subMeshBuffer,
		ArenaVector<uint32_t>& indexBuffer);
};

#endif