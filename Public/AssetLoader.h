#ifndef ASSETLOADER_H
#define ASSETLOADER_H

#include <string>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "ArenaAllocator.hpp"
#include "GlobalMacros.h"
#include "Log.hpp"
#include "Mesh.hpp"
#include "ErrorCodes.hpp"
#include "HlslTypes.h"
#include "Texture.hpp"


class RenderManager;
class Material;
struct aiMaterial;

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
		//ArenaVector<MeshData>& meshes,
		ArenaVector<BaseVSIn>& vertexBuffer,
		ArenaVector<MeshData>& subMeshBuffer,
		ArenaVector<uint32_t>& indexBuffer,
		RenderManager& render,
		MeshLoadInfo* outMeshInfo,
		std::vector<std::string>* subMeshNames);

	static Material& createMaterial(const aiMaterial* aiMat);

	static BaseMaterialInstance aiMaterialToBaseMaterialData(const aiMaterial* const aiMat);


	static void parseMaterialTextures(const std::string& filename, const aiScene* scene, const aiMaterial* mat, TexturePack& texPack);

	static void fillTexturePack(TexturePack& texPack, TextureType type, uint32_t index);
};

#endif