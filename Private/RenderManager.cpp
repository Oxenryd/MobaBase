#include "RenderManager.h"
#include <fstream>
#include "FileSys.h"
#include "Log.hpp"


RenderManager* RenderManager::s_instance = nullptr;

RenderManager::RenderManager(VulkanContext* const vkContext, const size_t paramArenaSize) :
	m_vkContext {vkContext},
	m_paramArena{ paramArenaSize }
{
	if (s_instance != nullptr)
		throw std::runtime_error("RenderManager already initialized.");
	s_instance = this;
}

Shader* RenderManager::getShader(const Shader::Type type, const size_t index) {
	switch (type) {
		case Shader::Type::Vertex:
		{
			if (index < m_vShaders.size())
				return &m_vShaders.at(index);
		} break;
		case Shader::Type::Fragment:
		{
			if (index < m_pShaders.size())
				return &m_pShaders.at(index);
		} break;
		case Shader::Type::Compute:
		{
			if (index < m_cShaders.size())
				return &m_cShaders.at(index);
		} break;
		default: std::unreachable();
	}

	return nullptr;
}

const Shader* RenderManager::getShader(
	const std::string& name) const {

	const auto it = m_shader_NamePairMap.find(name);
	if (it != m_shader_NamePairMap.end()) {
		const auto type = it->second.first;
		const auto index = it->second.second;
		switch (type) {
			case Shader::Type::Vertex:
				return const_cast<Shader*>(&m_vShaders[index]);
			case Shader::Type::Fragment:
				return const_cast<Shader*>(&m_pShaders[index]);
			case Shader::Type::Compute:
				return const_cast<Shader*>(&m_cShaders[index]);
			default: std::unreachable();
		}
	}
	return nullptr;
}

Shader* RenderManager::getShader(const std::string& name) {
	const auto it = m_shader_NamePairMap.find(name);
	if (it != m_shader_NamePairMap.end()) {
		const auto type = it->second.first;
		const auto index = it->second.second;
		switch (type) {
			case Shader::Type::Vertex:
				return const_cast<Shader*>(&m_vShaders[index]);
			case Shader::Type::Fragment:
				return const_cast<Shader*>(&m_pShaders[index]);
			case Shader::Type::Compute:
				return const_cast<Shader*>(&m_cShaders[index]);

			default: return nullptr;
		}
	}
	return nullptr;
}

const std::string& RenderManager::getShaderName(const Shader& shader) {
	const auto pair = std::pair{ shader.type, shader.m_arrayIndex };
	const auto it = m_shader_PairNameMap.find(pair);
	if (it != m_shader_PairNameMap.end()) {
		return it->second;
	} else {
		throw std::invalid_argument("RenderManager::getShaderName(): shader name not found.");
		//return std::string{ "INVALID SHADER AND THIS STRING SHOULD NEVER BE SEEN." };
	}
}

//const std::string& ShaderManager::getShaderName(const Shader::Type type, const size_t index) {
//	auto pair = std::pair{type, index};
//	auto it = m_shader_PairNameMap.find(pair);
//	if (it != m_shader_PairNameMap.end()) {
//		return it->second;
//	} else {
//		return std::string{ "INVALID SHADER AND THIS STRING SHOULD NEVER BE SEEN." };
//	}
//}

Material& RenderManager::createMaterial(const std::string& name, Shader& vertex, Shader& fragment) {
	Material newMaterial{};
	const auto matPtr = registerMaterial(name, newMaterial);
	auto actualMat = Material{ *matPtr, name, vertex, fragment };
	return *matPtr;
}

Material* RenderManager::getMaterial(const std::string& name) {
	const auto it = m_mat_NameIndexMap.find(name);
	if (it != m_mat_NameIndexMap.end()) {
		return &m_materials[it->second];
	} else
		return nullptr;
}

Material* RenderManager::getMaterial(const size_t& index) {
	const auto it = m_mat_IndexNameMap.find(index);
	if (it != m_mat_IndexNameMap.end()) {
		return &m_materials[index];
	} else
		return nullptr;
}

std::string& RenderManager::getMaterialName(const Material& mat) {
	return m_mat_IndexNameMap[mat.matIndex];
}

std::string& RenderManager::getTextureName(const Texture2D& tex) {
	return m_texIndexNameMap[tex.textureIndex];
}

Texture2D* RenderManager::getTexture(const std::string& filePath) {
	const auto it = m_texNameIndexMap.find(filePath);
	if (it != m_texNameIndexMap.end()) {
		return &m_tex2ds[it->second];
	} else
		return nullptr;
}

Texture2D* RenderManager::getTexture(const size_t texIndex) {
	const auto it = m_texIndexNameMap.find(texIndex);
	if (it != m_texIndexNameMap.end()) {
		return &m_tex2ds[texIndex];
	} else
		return nullptr;
}

//std::string& ShaderManager::getMaterialName(const size_t matIndex) {
//	return 
//}

ErrorCode RenderManager::recompileShaderCache() {

	// Find all HLSL files in directory
	std::vector<std::filesystem::path> hlslFiles;
	try {
		const auto workDir = FileSys::getExecutableDir() / SHADER_SOURCE_DIR;
		hlslFiles = FileSys::getAllFilesWithExtension(workDir, L".hlsl");
		if (hlslFiles.size() == 0)
			return ErrorCode::FILE_NOT_FOUND;
	} catch (std::exception e) {
		return ErrorCode::FILE_READ_ERROR;
	}

	for (auto& file : hlslFiles) {
		const auto type = FileSys::parseShaderTypeFromFile(file);
		if (type == Shader::Type::Invalid)
			continue;
		ErrorCode EC{};
		Shader newShader{ type, file };
		EC_CHECK(EC, compileShader(newShader));

		auto name = FileSys::parseShaderNameFromFile(file);
		EC_CHECK(EC, registerShader(name, newShader));
	}

	return ErrorCode::OK;
}

ErrorCode RenderManager::registerShader(const std::string& name, Shader& shader) {
	if (m_shader_NamePairMap.contains(name))
		return ErrorCode::SHADER_NAME_ALREADY_EXISTS;

	switch (shader.type) {
		case Shader::Type::Fragment:
			shader.m_arrayIndex = m_pShaders.size();
			m_pShaders.push_back(shader);
			m_pShaders.back().m_arrayIndex = shader.m_arrayIndex;
			break;

		case Shader::Type::Vertex:
			shader.m_arrayIndex = m_vShaders.size();
			m_vShaders.push_back(shader);
			m_vShaders.back().m_arrayIndex = shader.m_arrayIndex;
			break;

		case Shader::Type::Compute:
			shader.m_arrayIndex = m_cShaders.size();
			m_cShaders.push_back(shader);
			m_cShaders.back().m_arrayIndex = shader.m_arrayIndex;
			break;

		default:
			return ErrorCode::SHADER_REGISTER_INVALID_TYPE;
	}
	m_shader_NamePairMap.insert({ name, {shader.type, shader.m_arrayIndex} });
	m_shader_PairNameMap.insert({ {shader.type, shader.m_arrayIndex}, name  });
	return ErrorCode::OK;
}

Material* RenderManager::registerMaterial(const std::string& name, Material& material) {
	if (m_mat_NameIndexMap.contains(name)) {
		LOGLINE(LogType::Warning, LogMod::Rendering, "registerMaterial: material already exists.");
		return nullptr;
	}

	m_mat_NameIndexMap.insert({ name, m_materials.size() });
	m_mat_IndexNameMap.insert({ m_materials.size() , name});
	material.matIndex = m_materials.size();
	m_materials.push_back(material);

	return &m_materials.back();
}



ErrorCode RenderManager::compileShaders() {
	return ErrorCode::OK;
}

ErrorCode RenderManager::compileShader(Shader& shader) {


	std::string target;
	switch (shader.type) {
		case Shader::Type::Vertex:   target = "vs_6_6"; break;
		case Shader::Type::Fragment: target = "ps_6_6"; break;
		case Shader::Type::Compute:  target = "cs_6_6"; break;
		default: std::unreachable();
	}

	static auto execDir = FileSys::getExecutableDir();
	static std::filesystem::path shaderDir = SHADER_SOURCE_DIR;
	static std::filesystem::path outDir = SHADER_TARGET_DIR;
	static std::string dxcBin;
	static bool pathSet = false;
#ifdef BUILD_WIN
	if (!pathSet) {
		dxcBin = execDir.string() + "/dxc.exe";
		pathSet = true;
	}
#else
	if (!pathSet) {
		std::string libPath = execDir.string();
		dxcBin = "LD_LIBRARY_PATH=" + libPath + " " + (execDir / "dxc").string();
		pathSet = true;
	}
#endif

	auto outPath = outDir / shader.sourcePath.filename().replace_extension(".spv");
	std::stringstream cmd;
	cmd << dxcBin;
	cmd << " -spirv -fspv-target-env=vulkan1.3";
	cmd << " -fvk-use-dx-layout";
	cmd << " -T " << target;
	cmd << " -E main";
	//cmd << " -O0"; // debug
	cmd << " -Zi";
	cmd << " -I " << shaderDir.string();
	cmd << " -Fo " << outPath.string();
	//cmd << " -D USE_VULKAN";
	cmd << " " << shader.sourcePath.string();
	
	//-fvk-use-dx-layout

	ErrorCode EC{};
	std::string cmpError{};
	EC = FileSys::execAndCapture(cmd.str(), cmpError);
	if (EC != ErrorCode::OK) {
		LOGLINE(LogType::Error, LogMod::Rendering, cmpError);
		return EC;
	}

	//std::ifstream spvFile(outPath, std::ios::ate | std::ios::binary);
	//if (!spvFile) {
	//	return ErrorCode::SHADER_COULD_NOT_READ_FILE;
	//}

	//try {
	//	//spvFile.seekg(0, std::ios::end);
	//	size_t fileSize = spvFile.tellg();
	//	spvFile.seekg(0);
	//	//spvFile.seekg(0, std::ios::beg);

	//	shader.bytecode.resize(fileSize);
	//	spvFile.read(shader.bytecode.data(), fileSize);
	//} catch (std::exception& e) {
	//	return ErrorCode::SHADER_COULD_NOT_READ_BYTECODE;
	//}

	std::ifstream spvFile(outPath, std::ios::ate | std::ios::binary);
	if (!spvFile) {
		return ErrorCode::SHADER_COULD_NOT_READ_FILE;
	}

	try {
		size_t fileSize = spvFile.tellg();
		if (fileSize % 4 != 0) {
			return ErrorCode::SHADER_INVALID_BYTECODE_ALIGNMENT;
		}
		spvFile.seekg(0);

		size_t wordCount = fileSize / sizeof(uint32_t);
		shader.bytecode.resize(wordCount); // vector<uint32_t>
		spvFile.read(reinterpret_cast<char*>(shader.bytecode.data()), fileSize);

		if (!spvFile || spvFile.gcount() != static_cast<long>(fileSize)) {
			return ErrorCode::SHADER_COULD_NOT_READ_BYTECODE;
		}
	} catch (std::exception& e) {
		LOGLINE(LogType::Error, LogMod::Rendering, std::format("Read error: {}", e.what()));
		return ErrorCode::SHADER_COULD_NOT_READ_BYTECODE;
	}

	EC = shader.reflect();
	if (EC != ErrorCode::OK)
		return EC;

	return ErrorCode::OK;
}

ErrorCode RenderManager::checkForShaderChanges(Shader& shader) {
	return ErrorCode::OK;
}

ErrorCode RenderManager::hotReload() {

	ErrorCode EC{};
	bool changed = false;
	for (auto& ps : pixelShaders()) {
		auto writeTime = std::filesystem::last_write_time(ps.sourcePath);
		if (ps.lastSourceChangedTime < writeTime) {
			LOGLINE(LogType::Info, LogMod::Rendering, "Recompiling PS: " + ps.sourcePath.string() + "... ");
			EC_CHECK(EC, compileShader(ps));
			LOG(LogType::Success, "Done.");
			changed = true;
			ps.lastSourceChangedTime = writeTime;
		}
	}
	for (auto& vs : vertexShaders()) {
		auto writeTime = std::filesystem::last_write_time(vs.sourcePath);
		if (vs.lastSourceChangedTime < writeTime) {
			LOGLINE(LogType::Info, LogMod::Rendering, "Recompiling VS: " + vs.sourcePath.string() + "... ");
			EC_CHECK(EC, compileShader(vs));
			LOG(LogType::Success, "Done.");
			changed = true;
			vs.lastSourceChangedTime = writeTime;
		}
	}
	for (auto& cs : computeShaders()) {
		auto writeTime = std::filesystem::last_write_time(cs.sourcePath);
		if (cs.lastSourceChangedTime < writeTime) {
			LOGLINE(LogType::Info, LogMod::Rendering, "Recompiling CS: " + cs.sourcePath.string() + "... ");
			EC_CHECK(EC, compileShader(cs));
			LOG(LogType::Success, "Done.");
			changed = true;
			cs.lastSourceChangedTime = writeTime;
		}
	}

	if (changed) {
		onShaderHotReloaded.notify(nullptr);
	}

	return ErrorCode::OK;
}

