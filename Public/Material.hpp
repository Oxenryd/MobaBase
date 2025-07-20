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


struct DescriptorSetLayoutKey
{
	uint32_t setIndex;
	std::vector<uint32_t> bindings;
	std::vector<VkShaderStageFlags> stageFlags;
	std::vector<uint32_t> counts;
	std::vector<VkDescriptorType> types;
	std::vector<uint32_t> nameIndices;
	bool operator==(const DescriptorSetLayoutKey& other) const = default;


	void sortByBinding() {
		const size_t count = bindings.size();
		std::vector<size_t> indices(count);
		std::iota(indices.begin(), indices.end(), 0);

		std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
			return bindings[a] < bindings[b];
				  });

		auto reorder = [&](auto& vec) {
			using T = typename std::decay_t<decltype(vec)>::value_type;
			std::vector<T> sorted;
			sorted.reserve(count);
			for (auto i : indices)
				sorted.push_back(std::move(vec[i]));
			vec = std::move(sorted);
		};

		reorder(bindings);
		reorder(types);
		reorder(nameIndices);
		reorder(counts);
		reorder(stageFlags);
	}
};

struct DescriptorSetLayoutKeyHash
{
	size_t operator()(const DescriptorSetLayoutKey& key) const {
		size_t h = key.setIndex;
		for (auto b : key.bindings)       h ^= std::hash<uint32_t>{}(b)+0x9e3779b9 + (h << 6) + (h >> 2);
		for (auto t : key.types)          h ^= std::hash<uint32_t>{}(t)+0x9e3779b9 + (h << 6) + (h >> 2);
		for (auto n : key.nameIndices)	  h ^= std::hash<uint32_t>{}(n)+0x9e3779b9 + (h << 6) + (h >> 2);
		for (auto s : key.stageFlags)	  h ^= std::hash<uint32_t>{}(s)+0x9e3779b9 + (h << 6) + (h >> 2);
		for (auto c : key.counts)		  h ^= std::hash<uint32_t>{}(c)+0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}
};

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
	std::unordered_map<VkBindPair, SamplerState, VkBindPairHash> samplerStates;
	std::unordered_map<uint32_t, DescriptorSetLayoutKey> descriptorSetKeys;
	std::vector<MaterialInstance> instances;
	std::vector<MaterialBuffer> buffers;

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
		auto index = instances.size();
		instances.emplace_back(MaterialInstance{ this, index });
		for (auto& buffer : buffers) {
			uint32_t pushedIndex = buffer.push_new();
			assert(pushedIndex == index && "createInstance(): index mismatch between instance index and buffer index.");
		}

		return instances.back();
	}

	void initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);

	bool operator==(const Material& other);
	bool operator!=(const Material& other) { return !(*this == other); }

	void debugPrintMaterialInfo();
	void debugPrintParameters(size_t& depth, std::vector<MatParam>& params);
};




#endif