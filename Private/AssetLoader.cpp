#include "AssetLoader.h"
//#include "RenderManager.h"
#include "Engine.h"
#include <format>

ErrorCode AssetLoader::loadModel(
	const std::string& filename,
	ArenaVector<MeshData>& meshes,
	ArenaVector<BaseVSIn>& vertexBuffer,
	ArenaVector<SubMesh>& subMeshBuffer,
	ArenaVector<uint32_t>& indexBuffer,
	RenderManager& render,
	MeshDescription* outMeshInfo) {

	LOGLINE(LogType::Info, LogMod::Assets, std::format("Loading '{}'... ", filename));

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
											 aiProcess_Triangulate 
											 | aiProcess_JoinIdenticalVertices
											 | aiProcess_GenNormals
											 | aiProcess_CalcTangentSpace
	);
	if (!scene)
		return _logReturnError(ErrorCode::ASSETS_IMPORT_ERROR, std::format("Assimp Error:  {}", importer.GetErrorString()));
	else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		return _logReturnError(ErrorCode::ASSETS_MODEL_INCOMPLETE, std::format("Assimp Error:  {}", importer.GetErrorString()));
	} else if (!scene->mRootNode)
		return _logReturnError(ErrorCode::ASSETS_MODEL_NO_ROOT, std::format("Assimp Error:  {}", importer.GetErrorString()));

	MeshData mesh{};
	mesh.firstSubMeshIndex = subMeshBuffer.size();
	mesh.subMeshCount = scene->mNumMeshes;
	size_t vertCount = 0;
	size_t indexCount = 0;
	uint32_t newMats = 0;
	uint32_t parsedMats = 0;

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		const aiMesh* aiMesh = scene->mMeshes[i];

		SubMesh subMesh{};
		subMesh.vertexOffset = vertexBuffer.size();

		// Vertices
		uint32_t submeshVertices = 0;
		for (size_t j = 0; j < aiMesh->mNumVertices; ++j) {
			BaseVSIn vertex{};
			vertex.pos = toVec3(aiMesh->mVertices[j]);
			vertex.normal = aiMesh->HasNormals() ? toVec3(aiMesh->mNormals[j]) : glm::vec3{ 0, 0, 0 };
			vertex.texCoord = aiMesh->HasTextureCoords(0) ? toVec2(aiMesh->mTextureCoords[0][j]) : glm::vec2{ 0,0 };
			if (aiMesh->HasTangentsAndBitangents()) {
				vertex.tangent = toVec3(aiMesh->mTangents[j]);
				vertex.binormal = toVec3(aiMesh->mBitangents[j]);
			}
			vertexBuffer.push_back(vertex);
			submeshVertices++;
			vertCount++;
		}
		subMesh.vertexCount = submeshVertices;

		// Indices
		subMesh.indexOffset = indexBuffer.size();
		for (size_t f = 0; f < aiMesh->mNumFaces; ++f) {
			const aiFace& face = aiMesh->mFaces[f];
			if (face.mNumIndices != 3) {
				LOGLINE(LogType::Warning, LogMod::Assets, "loadModel(): face not triangulated? Skipping... ");
				continue;
			}
			for (size_t id = 0; id < 3; ++id) {
				indexBuffer.push_back(face.mIndices[id]);
				indexCount++;
			}
			
		}
		subMesh.indexCount = static_cast<uint32_t>(indexBuffer.size()) - subMesh.indexOffset;
		

		// Material
		const aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
		if (aiMat) {
			auto material = render.getMaterial(aiMat->GetName().C_Str());
			if (!material) {
				auto& newMat = createMaterial(aiMat);
				subMesh.materialIndex = newMat.matIndex;
				newMats++;
				
				auto matData = aiMaterialToBaseMaterialData(aiMat);
				newMat.createInstance(&matData);
				RenderManager::getInstance()->vkContext()->createPipelineFromMaterial(RenderManager::getInstance(), newMat);

			} else {
				subMesh.materialIndex = material->matIndex;
			}
			parsedMats++;
		} else {
			subMesh.materialIndex = UINT32_INVALID;
		}

		subMeshBuffer.push_back(subMesh);
	}
	if (outMeshInfo) {
		outMeshInfo->meshIndex = meshes.size();
		outMeshInfo->subMeshCount = subMeshBuffer.size() - mesh.firstSubMeshIndex;
		outMeshInfo->subMeshOffset = mesh.firstSubMeshIndex;
		//outMeshInfo->vertexOffset = subMeshBuffer[mesh.firstSubMeshIndex].vertexOffset;
		//outMeshInfo->vertexCount = vertCount;
		//outMeshInfo->indexOffset = subMeshBuffer[mesh.firstSubMeshIndex].indexOffset;
		//outMeshInfo->indexCount = indexCount;
	}
	meshes.push_back(mesh);

	
	LOGLINE(LogType::Info, LogMod::Assets, std::format("\t{} meshes, {}/{} materials/new, {} total vertices... ",
													   mesh.subMeshCount, parsedMats, newMats, vertCount));
	LOG(LogType::Success, "Done.");
	return ErrorCode::OK;
}

Material& AssetLoader::createMaterial(const aiMaterial* aiMat) {
	auto* baseVs = Engine::getInstance()->getRenderManager()->getShader(MAT_BASE_VS);
	auto* basePs = Engine::getInstance()->getRenderManager()->getShader(MAT_BASE_PS);
	std::string matName = aiMat->GetName().C_Str();
	auto& mat = Material::createMaterial(matName, *baseVs, *basePs );
	mat.debugPrintMaterialInfo();
	return mat;
}

BaseMaterialInstance AssetLoader::aiMaterialToBaseMaterialData(const aiMaterial* const aiMat) {
	BaseMaterialInstance base{};
	const aiMaterial& ai = *aiMat;
	char ptr[32]{ 0 };
	
	//TODO:
	// check textures -> load textures from filename -> bind indices
	
	if (ai.Get(AI_MATKEY_COLOR_AMBIENT, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = *reinterpret_cast<aiColor3D*>(ptr);
		base.ambient = {aiCol.r, aiCol.g, aiCol.b};
	}
	if (ai.Get(AI_MATKEY_COLOR_DIFFUSE, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = *reinterpret_cast<aiColor3D*>(ptr);
		base.baseColor = { aiCol.r, aiCol.g, aiCol.b };
	}
	if (ai.Get(AI_MATKEY_COLOR_SPECULAR, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = *reinterpret_cast<aiColor3D*>(ptr);
		base.specular = { aiCol.r, aiCol.g, aiCol.b };
	}
	if (ai.Get(AI_MATKEY_COLOR_EMISSIVE, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = *reinterpret_cast<aiColor3D*>(ptr);
		base.emissive = { aiCol.r, aiCol.g, aiCol.b };
	}
	if (ai.Get(AI_MATKEY_BASE_COLOR, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = *reinterpret_cast<aiColor4D*>(ptr);
		base.baseColor = { aiCol.r, aiCol.g, aiCol.b };
		base.transparency = aiCol.a;
	}
	if (ai.Get(AI_MATKEY_COLOR_TRANSPARENT, *reinterpret_cast<aiColor3D*>(ptr)) == AI_SUCCESS) {
		auto aiCol = reinterpret_cast<aiColor3D*>(ptr);
		base.transparentColor = { aiCol->r, aiCol->g, aiCol->b };
	}

	if (ai.Get(AI_MATKEY_SHININESS, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.shininess = *ai;
	}
	if (ai.Get(AI_MATKEY_SHININESS_STRENGTH, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.specularStrength = *ai;
	}
	if (ai.Get(AI_MATKEY_OPACITY, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.transparency = *ai;
	}
	if (ai.Get(AI_MATKEY_REFRACTI, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.refraction = *ai;
	}
	if (ai.Get(AI_MATKEY_METALLIC_FACTOR, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.metallic = *ai;
	}
	if (ai.Get(AI_MATKEY_ROUGHNESS_FACTOR, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.roughness = *ai;
	}
	if (ai.Get(AI_MATKEY_REFLECTIVITY, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.reflectivity = *ai;
	}
	if (ai.Get(AI_MATKEY_TRANSMISSION_FACTOR, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.transmission = *ai;
	}
	if (ai.Get(AI_MATKEY_EMISSIVE_INTENSITY, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.emissiveStrength = *ai;
	}
	if (ai.Get(AI_MATKEY_CLEARCOAT_FACTOR, *reinterpret_cast<ai_real*>(ptr)) == AI_SUCCESS) {
		auto ai = reinterpret_cast<ai_real*>(ptr);
		base.clearcoatStrength = *ai;
	}


	return base;
}
