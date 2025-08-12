#include "AssetLoader.h"
#include "Engine.h"
#include <format>

#ifndef STB_IMAGE_IMPLEMENTATION
	#define STB_IMAGE_IMPLEMENTATION
#endif

#include "stb_image.h"


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
		std::set<uint32_t> matIndices;
		const aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
		if (aiMat) {

			auto material = render.getMaterial(aiMat->GetName().C_Str());
			if (!material) {
				auto& newMat = createMaterial(aiMat);
				subMesh.materialIndex = newMat.matIndex;
				newMats++;
				
				auto matData = aiMaterialToBaseMaterialData(aiMat);
				parseMaterialTextures(filename, scene, aiMat, matData.textures);
				auto& matInstance = newMat.createInstance(&matData);
				subMesh.instanceIndex = matInstance.instanceIndex();
				auto pipelineResult = RenderManager::getInstance()->vkContext()->createPipelineFromMaterial(RenderManager::getInstance(), newMat);
				if (pipelineResult != VK_SUCCESS) {
					LOGLINE(LogType::Error, LogMod::Vulkan, "PipeLine creation failed.. ");
					return ErrorCode::VULKAN_COULD_NOT_CREATE_PIPELINE;
				}
			} else {
				subMesh.materialIndex = material->matIndex;
				auto& instance = material->createCopyOfLastInstance();
				subMesh.instanceIndex = instance.instanceIndex();
			}

		} else {
			// Fallback
			subMesh.materialIndex = 0;
			auto basicMaterial = render.getMaterial(0);
			auto& baseMatInstance = basicMaterial->createInstance();
			subMesh.instanceIndex = baseMatInstance.instanceIndex();
		}

		subMeshBuffer.push_back(subMesh);
	}
	if (outMeshInfo) {
		outMeshInfo->meshIndex = meshes.size();
		outMeshInfo->subMeshCount = subMeshBuffer.size() - mesh.firstSubMeshIndex;
		outMeshInfo->subMeshOffset = mesh.firstSubMeshIndex;
	}
	meshes.push_back(mesh);

	LOGLINE(LogType::Info, LogMod::Assets, std::format("\t{} meshes, {}/{} materials/new, {} total vertices... ",
													   mesh.subMeshCount, scene->mNumMaterials, newMats, vertCount));
	LOG(LogType::Success, "Done.");
	return ErrorCode::OK;
}

Material& AssetLoader::createMaterial(const aiMaterial* aiMat) {
	std::string matName = aiMat->GetName().C_Str();
	LOGLINE(LogType::Info, LogMod::Assets, std::format("Creating material {}... ", matName));
	auto* baseVs = Engine::getInstance()->getRenderManager()->getShader(SHADER_BASE_VS);
	auto* basePs = Engine::getInstance()->getRenderManager()->getShader(SHADER_BASE_PS);	
	auto& mat = Material::createMaterial(matName, *baseVs, *basePs );
	//mat.createInstance();
	LOG(LogType::Success, "Done.");
	return mat;
}

BaseMaterialInstance AssetLoader::aiMaterialToBaseMaterialData(const aiMaterial* const aiMat) {
	BaseMaterialInstance base{};
	const aiMaterial& ai = *aiMat;
	char ptr[32]{ 0 };
		
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

void AssetLoader::parseMaterialTextures(const std::string& filename, const aiScene* scene, const aiMaterial* mat, TexturePack& texPack) {

	for (const aiTextureType& aiType : Texture2D::aiTypesList) {
		size_t texCount = mat->GetTextureCount(aiType);
		if (texCount == 0)
			continue;
		aiString path;
		if (mat->GetTexture(aiType, 0, &path) != AI_SUCCESS)
			continue;

		auto texPtr = Engine::getInstance()->getRenderManager()->getTexture(path.C_Str());
		if (texPtr) {
			
			fillTexturePack(texPack, texPtr->type, texPtr->textureIndex);

			continue;
		}

		auto& texels = Engine::getInstance()->getRenderManager()->texels();
		Texture2D newTex{};
		newTex.type = Texture2D::typeFrom_aiTextureType(aiType);
		newTex.texelOffset = texels.size();

		// Texturedata baked into the scene
		if (path.length > 0 && path.C_Str()[0] == '*') {
			int texIndex = std::atoi(path.C_Str() + 1);
			if (texIndex >= 0 && texIndex < (int)scene->mNumTextures) {
				const aiTexture* embeddedTex = scene->mTextures[texIndex];

				if (embeddedTex->mHeight == 0) {
					// Compressed texture (PNG/JPEG) in memory
					size_t dataSize = embeddedTex->mWidth;
					const uint8_t* data = reinterpret_cast<const uint8_t*>(embeddedTex->pcData);
					// Now you can feed `data` to your image loader (stb_image, etc.)
					throw std::exception("Not Implemented.");

				} else {
					

					newTex.height = embeddedTex->mHeight;
					newTex.width = embeddedTex->mWidth;
					newTex.filePath = path.C_Str();

					newTex = Engine::getInstance()->getRenderManager()->registerTexture(path.C_Str(), newTex);
					fillTexturePack(texPack, newTex.type, newTex.textureIndex);
					const aiTexel* pixels = embeddedTex->pcData;
					for (size_t i = 0; i < newTex.texelCount(); ++i) {
						texels.push_back(ColorRGBA{ pixels[i] });
					}
				}
			}
			continue;
		}



		// Data needs to be fetched from file
		int w, h, chan;
		std::filesystem::path fullPath = std::filesystem::path(filename).parent_path() / path.C_Str();
		std::string pathString = fullPath.generic_string();
		unsigned char* data = stbi_load(
			pathString.c_str(),
			&w, &h, &chan, 4
		);
		if (data) {
			newTex.filePath = pathString;
			newTex.width = static_cast<uint16_t>(w);
			newTex.height = static_cast<uint16_t>(h);
			newTex = Engine::getInstance()->getRenderManager()->registerTexture(path.C_Str(), newTex);
			fillTexturePack(texPack, newTex.type, newTex.textureIndex);
			
			//for (size_t i = 0; i < newTex.texelCount(); ++i) {
			//	texels.push_back(*reinterpret_cast<ColorRGBA*>(data + i));
			//}
			auto* pixels = reinterpret_cast<const ColorRGBA*>(data);
			texels.insert(texels.end(), pixels, pixels + newTex.texelCount());

			stbi_image_free(data);
			LOGLINE(LogType::Info, LogMod::Assets, std::format("Imported Texture '{}'", pathString));
		} else
			LOGLINE(LogType::Error, LogMod::Assets, std::format("stb_image: Failed to load file: '{}'", pathString));
	}
}

void AssetLoader::fillTexturePack(TexturePack& texPack, TextureType type, uint32_t index) {

	switch (type) {
		case TextureType::Diffuse:
			texPack.albedoId = index; break;

		case TextureType::Emissive:
			texPack.emissiveId = index; break;

		case TextureType::Clearcoat:
			break;

		case TextureType::Metalness:
			texPack.metallicId = index; break;

		case TextureType::AmbientOcclusion:
			texPack.aoId = index; break;

		case TextureType::Height:
		case TextureType::Normal:
			texPack.normalId = index; break;

		case TextureType::Specular:
			texPack.specularId = index; break;

		case TextureType::Roughness:
			texPack.roughnessId = index; break;

		default:
			break;
	}
}
