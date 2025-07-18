#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <map>

#include "ErrorCodes.hpp"
#include "MaterialParams.h"


#include "Texture.hpp"
#include "MaterialStates.h"
#include "ArenaAllocator.hpp"


class Material;
class MaterialInstance
{
private:
	uint32_t m_instanceIndex;

public:
	MaterialInstance(Material* const base, size_t index) :
		base{ base },
		m_instanceIndex{static_cast<uint32_t>(index)}
	{}
	Material* const base;
	void* const pushDataPtr() { return nullptr; }
};


class Shader;
class ShaderManager;
class VulkanContext;
struct ShaderBinding;
struct PsoDesc;

class Material
{
private:
	friend ShaderManager;
	friend VulkanContext;
	
	//void fromJson(const json& j);
	void addShaderParams(
		std::vector<MatParam>& params,
		std::vector<ShaderBinding>& s,
		MatParamStage stage,
		const std::string& parentName);

	void setParamStageRecursive(MatParam& param, MatParamStage stage) {
		param.stage = stage;
		for (auto& p : param.members) {
			setParamStageRecursive(p, stage);
		}
	}

	void init(VkPipeline& pipeline, VkPipelineLayout& layout, uint32_t id) {
		this->pipeline = pipeline;
		this->pipelineLayout = layout;
		pipelineId = id;
	}

public:
	size_t arrayIndex = SIZE_T_INVALID;
	std::map<std::string, size_t> paramsMap;
	std::vector<MatParam> params;
	std::string vShaderName;
	std::string pShaderName;
	std::vector<void*> blendModeCustomPtrs;
	std::vector<BlendMode> blendModes;
	void* depthModeCustomPtr = nullptr;
	void* rasterModeCustomPtr = nullptr;
	void* msaaModeCustomPtr = nullptr;
	DepthMode depthMode = DepthMode::None;
	RasterMode rasterMode = RasterMode::RasterDefault;
	MultiSamplingMode msaaMode = MultiSamplingMode::MSAA_None;
	uint32_t pipelineId = UINT32_INVALID;
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	std::vector<VkDescriptorSet> sets;
	uint32_t pushConstantOffset = 0;
	uint32_t pushConstantSize = 0;
	VkShaderStageFlags pushShaderFlags{};
	std::vector<SamplerState> samplerStates;
	std::vector<VarTypeStructDefinition> bufferDefinitions;
	std::vector<VarTypeStructDefinition> pushConstantDefinition;
	std::vector<MaterialInstance> instances;

	Material()
	{
		blendModes.push_back(BlendMode::Premultiplied);
		blendModeCustomPtrs.push_back(nullptr);
	};
	//Material(const json& j) { fromJson(j); }
	Material(const std::string& name, Shader& vertex, Shader& fragment)
	{
		blendModes.push_back(BlendMode::Premultiplied);
		blendModeCustomPtrs.push_back(nullptr);
		initFromShaders(name, vertex, fragment);
	}

	std::string& name();
	std::string& name() const;
	bool isInitialized() const {
		return
			arrayIndex != SIZE_T_INVALID &&
			pipelineId != UINT32_INVALID &&
			pipeline && pipelineLayout;
	}

	//json toJson();
	
	MaterialInstance& createInstance() {
		instances.emplace_back(MaterialInstance{ this, instances.size()});
		

		return instances.back();
	}

	void initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);

	bool operator==(const Material& other);
	bool operator!=(const Material& other) { return !(*this == other); }

	void debugPrintMaterialInfo();
	void debugPrintParameters(size_t& depth, std::vector<MatParam>& params);
};




#endif