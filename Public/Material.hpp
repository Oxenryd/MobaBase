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

using DrawIndex = uint32_t;
using MaterialInstanceIndex = uint32_t;
using InstanceIndices = uint32_t*;
using InstanceCount = uint32_t;

using MaterialCallbackSingleDraw = void(*)(Material&, DrawIndex, MaterialInstanceIndex);
using MaterialCallbackInstancedDraw = void(*)(Material&, InstanceIndices, InstanceCount);
using MaterialCallback = void(*)(Material&, MaterialInstanceIndex);

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

struct PipelineLayoutKey
{
	std::vector<VkDescriptorSetLayout> setLayouts;
	std::vector<VkPushConstantRange> pushConstants;

	bool operator==(const PipelineLayoutKey& other) const = default;
};

struct PipelineLayoutKeyHash
{
	size_t operator()(const PipelineLayoutKey& key) const {
		size_t seed = key.setLayouts.size();
		for (auto layout : key.setLayouts)
			seed ^= std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(layout)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		for (auto& pc : key.pushConstants) {
			seed ^= std::hash<uint32_t>{}(pc.offset);
			seed ^= std::hash<uint32_t>{}(pc.size);
			seed ^= std::hash<uint32_t>{}(pc.stageFlags);
		}
		return seed;
	}
};


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
	MaterialBuffer* getBuffer(const std::string& bufferName);
	void setParameter(const std::string& bufferName, const std::string& paramName, void* value);
	template <typename T>
	T* getParameter(const std::string& bufferName, const std::string& paramName) {
		auto buffer = getBuffer(bufferName);
		if (buffer) {
			return buffer->getParameter<T>(paramName, m_instanceIndex);
		}
		return nullptr;
	}
	void* const pushDataPtr() { return nullptr; }
};

class Shader;
class RenderManager;
class VulkanContext;
struct ShaderBinding;
struct PsoDesc;

class Material
{
private:
	friend RenderManager;
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
	size_t matIndex = SIZE_INVALID;
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
	std::unordered_map<uint32_t, size_t> bufferNameIndexMap;
	MaterialCallbackSingleDraw preDraw = nullptr;
	MaterialCallbackInstancedDraw preDrawInstanced = nullptr;
	MaterialCallback preBind = nullptr;
	MaterialCallback postDraw = nullptr;

	~Material() = default;
	Material()
	{
		blendModes.push_back(BlendMode::Premultiplied);
		blendModeCustomPtrs.push_back(nullptr);
	};
	//Material(const json& j) { fromJson(j); }
	Material(const std::string& name, Shader& vertex, Shader& fragment) : 
		Material{ initFromShaders(name, vertex, fragment) } {
	
		//DEBUG
		createInstance();
	}


	std::string& name();
	std::string& name() const;
	bool isInitialized() const {
		return
			matIndex != SIZE_INVALID &&
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

	MaterialBuffer* getBuffer(const std::string& bufferName);
	

	static Material& initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);

	bool operator==(const Material& other);
	bool operator!=(const Material& other) { return !(*this == other); }

	void debugPrintMaterialInfo();
	void debugPrintParameters(size_t& depth, std::vector<MatParam>& params);
};




#endif