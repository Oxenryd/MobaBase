#include "ShaderManager.h"
#include <fstream>
#include "FS.h"
#include "Log.hpp"

ShaderManager::ShaderManager(ShaderCompilerBase* compiler) :
	m_compiler{ compiler }
{
}

Shader* const ShaderManager::getShader(Shader::Type type, size_t index) {
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
	}

	return nullptr;
}

ErrorCode ShaderManager::recompileShaderCache() {

	// Find all HLSL files in directory
	std::vector<std::filesystem::path> hlslFiles;
	try {
		auto workDir = FS::getExecutableDir() / SHADER_SOURCE_DIR;
		hlslFiles = FS::getAllFilesWithExtension(workDir, L".hlsl");
		if (hlslFiles.size() == 0)
			return ErrorCode::FILE_NOT_FOUND;
	} catch (std::exception e) {
		return ErrorCode::READ_ERROR;
	}

	for (auto& file : hlslFiles) {
		auto type = FS::parseShaderTypeFromFile(file);
		if (type == Shader::Type::Invalid)
			continue;
		ErrorCode EC{};
		Shader newShader{ type, file };
		EC_CHECK(EC, m_compiler->compile(newShader));

		switch (type) {
			case Shader::Type::Vertex:
				SetShaderArrayIndex(newShader, m_vShaders.size());
				m_vShaders.push_back(newShader);
				break;

			case Shader::Type::Fragment:
				SetShaderArrayIndex(newShader, m_pShaders.size());
				m_pShaders.push_back(newShader);
				break;

			case Shader::Type::Compute:
				SetShaderArrayIndex(newShader, m_cShaders.size());
				m_cShaders.push_back(newShader);
				break;
		}
	}

	return ErrorCode::OK;
}

ErrorCode DxcWin32VulkanShaderCompiler::compileShaders(ShaderManager*) {
	return ErrorCode::OK;
}

ErrorCode DxcWin32VulkanShaderCompiler::compile(Shader& shader) {

	std::string target;
	switch (shader.type) {
		case Shader::Type::Vertex:   target = "vs_6_6"; break;
		case Shader::Type::Fragment: target = "ps_6_6"; break;
		case Shader::Type::Compute:  target = "cs_6_6"; break;
	}
	auto execDir = FS::getExecutableDir();
	std::filesystem::path shaderDir = SHADER_SOURCE_DIR;
	std::filesystem::path dxcDir = DXC_DIR;// / "dxc.exe";
	std::filesystem::path outDir = SHADER_TARGET_DIR;
	auto outPath = outDir / shader.sourcePath.filename().replace_extension(".spv");
	std::stringstream cmd;
	cmd << dxcDir.string() << "/dxc.exe";
	cmd << " -spirv -fspv-target-env=vulkan1.3";
	cmd << " -T " << target;
	cmd << " -E main";
	cmd << " -I " << shaderDir.string();
	cmd << " -Fo " << outPath.string();
	//cmd << " -D USE_VULKAN";
	cmd << " " << shader.sourcePath.string();
	

	ErrorCode EC{};
	std::string cmpError{};
	EC = FS::execAndCapture(cmd.str(), cmpError);
	if (EC != ErrorCode::OK) {
		LOGLINE(LogType::Error, LogMod::Rendering, cmpError);
		return EC;
	}

	std::ifstream spvFile(outPath, std::ios::ate | std::ios::binary);
	if (!spvFile) {
		return ErrorCode::SHADER_COULD_NOT_READ_FILE;
	}

	try {
		//spvFile.seekg(0, std::ios::end);
		size_t fileSize = spvFile.tellg();
		spvFile.seekg(0);
		//spvFile.seekg(0, std::ios::beg);

		shader.bytecode.resize(fileSize);
		spvFile.read(shader.bytecode.data(), fileSize);
	} catch (std::exception& e) {
		return ErrorCode::SHADER_COULD_NOT_READ_BYTECODE;
	}

	shader.lastWriteTime = std::filesystem::last_write_time(shader.sourcePath);
	return ErrorCode::OK;
}

ErrorCode DxcWin32VulkanShaderCompiler::checkForShaderChanges(Shader& shader) {
	return ErrorCode::OK;
}

ErrorCode DxcWin32VulkanShaderCompiler::hotReload(ShaderManager& manager) {

	ErrorCode EC{};
	for (auto& ps : manager.pixelShaders()) {
		if (ps.lastWriteTime < std::filesystem::last_write_time(ps.sourcePath)) {
			EC_CHECK(EC, compile(ps));
		}
	}
	for (auto& vs : manager.vertexShaders()) {
		if (vs.lastWriteTime < std::filesystem::last_write_time(vs.sourcePath)) {
			EC_CHECK(EC, compile(vs));
		}
	}
	for (auto& cs : manager.computeShaders()) {
		if (cs.lastWriteTime < std::filesystem::last_write_time(cs.sourcePath)) {
			EC_CHECK(EC, compile(cs));
		}
	}

	return ErrorCode::OK;
}

