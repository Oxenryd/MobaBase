#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <vector>
#include <unordered_map>
#include <utility>
#include <queue>

#include "ErrorCodes.hpp"
#include "Timer.h"
#include "Delegate.hpp"

#include "IShaderProvider.h"
#include "Material.hpp"
#include "Hashes.hpp"
#include "ArenaAllocator.hpp"
#include "GlobalMacros.h"
#include "HlslTypes.h"
#include "Transform.hpp"

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
using MatrixIndex = uint32_t;
class Engine;
class ShaderManager : public IShaderProvider
{
private:
	friend Engine;
	static ShaderManager* s_instance;
	ShaderCompilerBase* const m_compiler;
	friend ShaderCompilerBase;
	std::vector<Shader> m_vShaders;
	std::vector<Shader> m_pShaders;
	std::vector<Shader> m_cShaders;
	std::vector<Material> m_materials;
	std::vector<std::string> m_paramNames;
	Timer* m_hotreloadTimer = nullptr;
	std::vector<ModelTransform> modelTransforms;
	std::queue<std::pair<MatrixIndex, TransformComponent*>> pendingMatrixUpdates;

	// Mapping
	std::unordered_map<std::string, size_t> m_param_NameIndexMap;
	std::unordered_map<std::string, size_t> m_mat_NameIndexMap;
	std::unordered_map<size_t, std::string> m_mat_IndexNameMap;
	std::unordered_map<std::string, std::pair<Shader::Type, Index>> m_shader_NamePairMap;
	std::unordered_map<std::pair<Shader::Type, Index>, std::string, PairHash<Shader::Type, size_t>> m_shader_PairNameMap;

	// Memory
	Arena m_paramArena;
	

public:
	ShaderManager() = delete;
	~ShaderManager() { delete m_hotreloadTimer; }
	ShaderManager(ShaderCompilerBase* compiler);

	ShaderCompilerBase* const getCompiler() { return m_compiler; }

	std::vector<Shader>& vertexShaders() { return m_vShaders; }
	std::vector<Shader>& pixelShaders() { return m_pShaders; }
	std::vector<Shader>& computeShaders() { return m_cShaders; }
	Shader* const getShader(Shader::Type type, size_t index);
	Shader* const getShader(const std::string& name) const override;
	Shader* const getShader(const std::string& name);
	const std::string& getShaderName(const Shader& shader);
	//const std::string& getShaderName(const Shader::Type type, const size_t index);
	ErrorCode recompileShaderCache();
	ErrorCode hotReload() { if (m_compiler) return m_compiler->hotReload(*this); return ErrorCode::MEMORY_IS_NULL; }
	size_t totalShaders() { return m_vShaders.size() + m_pShaders.size() + m_cShaders.size(); }
	ErrorCode registerShader(const std::string& name, Shader& shader);

	std::vector<Material>& materials() { return m_materials; }
	Material* getMaterial(const std::string& name);
	Material* getMaterial(const size_t& index);
	std::string& getMaterialName(const Material& mat);

	//std::string& getMaterialName(const size_t matIndex);
	size_t getParamNameIndex(const std::string& name) {
		auto it = m_param_NameIndexMap.find(name);
		if (it != m_param_NameIndexMap.end())
			return it->second;
		
		return SIZE_T_INVALID;
	}
	std::string& getParamName(size_t index) { return m_paramNames[index]; }
	const std::string& getParamName(size_t index) const override { return const_cast<std::string&>(m_paramNames[index]); }
	size_t registerParamName(const std::string& name) {
		auto index = m_paramNames.size();
		m_param_NameIndexMap.insert({name, index});
		m_paramNames.push_back(name);
		return index;
	}
	std::vector<std::string>& paramNames() { return m_paramNames; }
	Material* registerMaterial(const std::string& name, Material& material);

	Event<void*> onShaderHotReloaded;

	// 'Singleton'
	static ShaderManager* getInstance() { 
		return s_instance;
	}
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