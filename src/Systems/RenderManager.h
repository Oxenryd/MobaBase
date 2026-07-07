#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <vector>
#include <unordered_map>
#include <utility>
#include <queue>

#include <boost/unordered/unordered_flat_map.hpp>

#include "ErrorCodes.hpp"
#include "Timer.h"
#include "Delegate.hpp"
#include "Shader.h"
#include "Material.hpp"
#include "Hashes.hpp"
#include "ArenaAllocator.hpp"
#include "GlobalMacros.h"
//#include "HlslTypes.h"
#include "Transform.hpp"
#include "BasicTypes.hpp"
#include "Texture.hpp"





class RenderManager;

class ShaderCompilerBase
{
public:
	virtual ~ShaderCompilerBase() {}
	virtual ErrorCode compileShaders(RenderManager*) = 0;
	virtual ErrorCode compile(Shader& shader) = 0;
	virtual ErrorCode checkForShaderChanges(Shader& shader) = 0;
	virtual ErrorCode hotReload(RenderManager& manager) = 0;
};

using Index = size_t;
class Engine;
class RenderManager// : public IRenderProvider
{
	friend Engine;
	static RenderManager* s_instance;
	std::vector<Shader> m_vShaders;
	std::vector<Shader> m_pShaders;
	std::vector<Shader> m_cShaders;
	std::vector<Material> m_materials;
	std::vector<std::string> m_paramNames;
	//std::vector<std::string> m_texNames;
	std::vector<Texture2D> m_tex2ds;
	//std::vector<VkTextureResource> m_texResources;
	std::vector<ColorRGBA> m_texels;

	Timer* m_hotreloadTimer = nullptr;
	VulkanContext* const m_vkContext = nullptr;
	//std::unordered_map<SceneIndex, std::vector<ModelTransform>> modelTransforms;
	//std::unordered_map<SceneIndex, std::queue<std::pair<MatrixIndex, TransformComponent*>>> pendingMatrixUpdates;

#define MAP_STRKEY(x) boost::unordered_flat_map<std::string, x, StringHash, std::equal_to<>>
#define Cma ,
	// Mapping
	MAP_STRKEY(size_t) m_param_NameIndexMap;
	MAP_STRKEY(size_t) m_mat_NameIndexMap;
	boost::unordered_flat_map<size_t, std::string, IntHash<size_t>> m_mat_IndexNameMap;
	MAP_STRKEY(std::pair<Shader::Type Cma Index>) m_shader_NamePairMap;
	boost::unordered_flat_map<std::pair<Shader::Type, Index>, std::string, PairHash<Shader::Type, size_t>> m_shader_PairNameMap;
	MAP_STRKEY(size_t) m_texNameIndexMap;
	boost::unordered_flat_map<std::size_t, std::string, IntHash<size_t>> m_texIndexNameMap;

#undef MAP_STRKEY
#undef Cma

	// Memory
	Arena m_paramArena;
	
	// Materials
	INLINE Material* registerMaterial(const std::string& name, Material& material);

public:
	RenderManager() = delete;
	~RenderManager() {
		delete m_hotreloadTimer;
		if (this != s_instance) {
			delete s_instance;
			s_instance = nullptr;
		}
	}
	RenderManager(VulkanContext* vkContext, size_t paramArenaSize);
	RenderManager(RenderManager&& other) = default;

	INLINE std::vector<Shader>& vertexShaders() { return m_vShaders; }
	INLINE std::vector<Shader>& pixelShaders() { return m_pShaders; }
	INLINE std::vector<Shader>& computeShaders() { return m_cShaders; }
	INLINE std::vector<Texture2D>& textures() { return m_tex2ds; }
	Shader* getShader(Shader::Type type, size_t index);
	const Shader* getShader(const std::string& name) const;
	Shader* getShader(const std::string& name);
	const std::string& getShaderName(const Shader& shader);
	//const std::string& getShaderName(const Shader::Type type, const size_t index);
	ErrorCode recompileShaderCache();
	INLINE size_t totalShaders() const { return m_vShaders.size() + m_pShaders.size() + m_cShaders.size(); }
	ErrorCode registerShader(const std::string& name, Shader& shader);


	INLINE std::span<ColorRGBA> getPixels(const Texture2D& texture) {
		return std::span{ &m_texels[texture.texelOffset], texture.texelCount() };
	}
	INLINE std::vector<ColorRGBA>& texels() { return m_texels; }
	INLINE std::vector<Material>& materials() { return m_materials; }
	//INLINE std::vector<VkTextureResource>& textureResources() { return m_texResources; }
	Material& createMaterial(const std::string& name, Shader& vertex, Shader& fragment);
	Material* getMaterial(const std::string& name);
	Material* getMaterial(const size_t& index);
	std::string& getMaterialName(const Material& mat);
	std::string& getTextureName(const Texture2D& tex);
	Texture2D* getTexture(const std::string& filePath);
	Texture2D* getTexture(size_t texIndex);
	//INLINE VkTextureResource* getTexResource(const size_t resourceIndex) {
	//	if (resourceIndex >= m_texResources.size())
	//		return nullptr;
	//	return &m_texResources[resourceIndex];
	//}


	//std::string& getMaterialName(const size_t matIndex);
	INLINE size_t getParamNameIndex(const std::string& name) {
		const auto it = m_param_NameIndexMap.find(name);
		if (it != m_param_NameIndexMap.end())
			return it->second;
		
		return SIZE_INVALID;
	}
	INLINE std::string& getParamName(const size_t index) { return m_paramNames[index]; }
	INLINE const std::string& getParamName(const size_t index) const  { return const_cast<std::string&>(m_paramNames[index]); }
	INLINE size_t registerParamName(const std::string& name) {
		auto index = m_paramNames.size();
		m_param_NameIndexMap.insert({name, index});
		m_paramNames.push_back(name);
		return index;
	}
	INLINE Texture2D& registerTexture(const std::string& name, const Texture2D& texture) {
		const auto index = m_tex2ds.size();
		m_tex2ds.push_back(texture);
		auto& newTex = m_tex2ds.back();
		newTex.textureIndex = static_cast<uint32_t>(index);
		const auto nameIt = m_texIndexNameMap.find(index);
		if (nameIt == m_texIndexNameMap.end()) {
			m_texIndexNameMap[index] = name;
			m_texNameIndexMap[name] = index;
		} else
			throw std::runtime_error("Error in texture indices!");
		return newTex;
	}
	std::vector<std::string>& paramNames() { return m_paramNames; }
	

	Event<void*> onShaderHotReloaded;

	// 'Singleton'
	static RenderManager* getInstance() { 
		return s_instance;
	}

	VulkanContext* vkContext() { return m_vkContext; }


	static ErrorCode compileShaders();
	static ErrorCode compileShader(Shader& shader);
	static ErrorCode checkForShaderChanges(Shader& shader);
	ErrorCode hotReload();
};

// class DxcWin32VulkanShaderCompiler final : public ShaderCompilerBase
// {
// public:
// 	~DxcWin32VulkanShaderCompiler() override {}
// 	ErrorCode compileShaders(RenderManager*) override;
// 	ErrorCode compile(Shader& shader) override;
// 	ErrorCode checkForShaderChanges(Shader& shader) override;
// 	ErrorCode hotReload(RenderManager& manager) override;
// };

#endif