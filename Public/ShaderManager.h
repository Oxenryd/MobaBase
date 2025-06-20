#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <vector>
#include <unordered_map>
#include <utility>

#include "Shader.hpp"
#include "ErrorCodes.hpp"
#include "Timer.h"
#include "Delegate.hpp"

class ShaderManager;

class ShaderCompilerBase
{
public:
	virtual ~ShaderCompilerBase() {}
	virtual ErrorCode compileShaders(ShaderManager*) = 0;
	virtual ErrorCode compile(Shader& shader) = 0;
	virtual ErrorCode checkForShaderChanges(Shader& shader) = 0;
	virtual ErrorCode hotReload(ShaderManager& manager) = 0;
};

using Index = size_t;

class ShaderManager
{
private:
	friend Engine;
	ShaderCompilerBase* const m_compiler;
	friend ShaderCompilerBase;
	std::unordered_map<std::string, std::pair<Shader::Type, Index>> m_shaderMap;
	std::vector<Shader> m_vShaders;
	std::vector<Shader> m_pShaders;
	std::vector<Shader> m_cShaders;
	Timer* m_hotreloadTimer = nullptr;

public:
	ShaderManager() = delete;
	~ShaderManager() {
		delete m_hotreloadTimer;
	}
	ShaderManager(ShaderCompilerBase* compiler);

	ShaderCompilerBase* const getCompiler() { return m_compiler; }
	Shader* const getShader(Shader::Type type, size_t index);
	ErrorCode recompileShaderCache();
	ErrorCode hotReload() { if (m_compiler) return m_compiler->hotReload(*this); return ErrorCode::IS_NULL; }
	size_t totalShaders() { return m_vShaders.size() + m_pShaders.size() + m_cShaders.size(); }

	std::vector<Shader>& vertexShaders() { return m_vShaders; }
	std::vector<Shader>& pixelShaders() { return m_pShaders; }
	std::vector<Shader>& computeShaders()  { return m_cShaders; }

	Event<void*> onShaderHotReloaded;
};

class DxcWin32VulkanShaderCompiler : public ShaderCompilerBase
{
public:
	~DxcWin32VulkanShaderCompiler() {}
	ErrorCode compileShaders(ShaderManager*) override;
	ErrorCode compile(Shader& shader) override;
	ErrorCode checkForShaderChanges(Shader& shader) override;
	ErrorCode hotReload(ShaderManager& manager) override;
};

#endif