#include "AssetLoader.h"
#include "RenderManager.h"
#include <format>

ErrorCode AssetLoader::loadModel(
	const std::string& filename,
	ArenaVector<MeshData>& meshes,
	ArenaVector<BaseVSIn>& vertexBuffer,
	ArenaVector<SubMesh>& subMeshBuffer,
	ArenaVector<uint32_t>& indexBuffer,
	RenderManager& render,
	uint32_t* outMeshIndex) {

	LOGLINE(LogType::Info, LogMod::Assets, std::format("Loading '{}'... ", filename));

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
											 aiProcess_Triangulate |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals |
											 aiProcess_CalcTangentSpace
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

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		const aiMesh* aiMesh = scene->mMeshes[i];

		SubMesh subMesh{};
		subMesh.vertexOffset = vertexBuffer.size();

		// Vertices
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
			vertCount++;
		}

		// Indices
		subMesh.indexOffset = indexBuffer.size();
		for (size_t f = 0; f < aiMesh->mNumFaces; ++f) {
			const aiFace& face = aiMesh->mFaces[f];
			if (face.mNumIndices != 3) {
				LOGLINE(LogType::Warning, LogMod::Assets, "loadModel(): face not triangulated? Skipping... ");
				continue;
			}
			for (size_t id = 0; id < 3; ++id)
				indexBuffer.push_back(face.mIndices[id]);
		}
		subMesh.indexCount = static_cast<uint32_t>(indexBuffer.size()) - subMesh.indexOffset;
		

		// Material
		const aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
		if (aiMat) {
			auto material = render.getMaterial(aiMat->GetName().C_Str());
			if (!material) {
				/* TODO: CREATE NEW MATERIAL HERE!! */ subMesh.materialIndex = UINT32_INVALID;
			} else {
				subMesh.materialIndex = material->matIndex;
			}
		} else {
			subMesh.materialIndex = UINT32_INVALID;
		}

		subMeshBuffer.push_back(subMesh);
	}
	if (outMeshIndex)
		*outMeshIndex = meshes.size();
	meshes.push_back(mesh);

	
	LOGLINE(LogType::Info, LogMod::Assets, std::format("\t{} meshes, {} materials, {} total vertices... ",
													   mesh.subMeshCount, scene->mNumMaterials, vertCount));
	LOG(LogType::Success, "Done.");
	return ErrorCode::OK;
}