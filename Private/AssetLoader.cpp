#include "AssetLoader.h"
#include "RenderManager.h"

ErrorCode AssetLoader::loadModel(
	const std::string& filename,
	ArenaVector<Mesh>& meshes,
	ArenaVector<BaseVSIn>& vertexBuffer,
	ArenaVector<SubMesh>& subMeshBuffer,
	ArenaVector<uint32_t>& indexBuffer) {

	LOGLINE(LogType::Info, LogMod::Assets, std::format("Loading '{}'... "));

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

	Mesh mesh{};
	mesh.firstSubMeshIndex = vertexBuffer.size();
	mesh.subMeshCount = scene->mNumMeshes;


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
		}

		// Indices
		for (size_t f = 0; f < aiMesh->mNumFaces; ++f) {
			const aiFace& face = aiMesh->mFaces[f];
			if (face.mNumIndices != 3) {
				LOGLINE(LogType::Warning, LogMod::Assets, "loadModel(): face not triangulated? Skipping... ");
				continue;
			}
			for (size_t id = 0; id < 3; ++id)
				indexBuffer.push_back(subMesh.vertexOffset + face.mIndices[id]);
		}
		subMesh.indexCount = static_cast<uint32_t>(indexBuffer.size()) - subMesh.indexOffset;
		subMeshBuffer.push_back(subMesh);

		// Material
		const aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
		if (material) {

		}
	}

	//for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
	//	const aiMaterial* material = scene->mMaterials[m];
	//	aiString name;
	//	material->Get(AI_MATKEY_NAME, name);
	//	std::cout << "Material " << m << ": " << name.C_Str() << "\n";
	//}

	meshes.push_back(mesh);
}