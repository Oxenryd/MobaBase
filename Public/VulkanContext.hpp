#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#pragma warning(push)
#pragma warning(disable : 28251)

//#include <csignal>
//#ifndef SIGTRAP
//#define SIGTRAP 5 // works for GCC/Clang, but not meaningful in MSVC
//#endif

#ifdef BUILD_WIN

	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif

#ifdef BUILD_GLFW
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan.hpp>

#include <thread>
#include <vector>
#include <array>
#include <unordered_map>
#include <queue>

#include "DrawCommand.hpp"
#include "WindowSurface.h"
#include "Log.hpp"
#include "Robin_Hood.h"


#include "Material.hpp"
#include "RenderManager.h"
#include "RenderTarget.hpp"
#include "HlslTypes.h"
#include "DrawCommand.hpp"

#include <variant>

#define Vk_FAILED(ec) ((ec) != VK_SUCCESS)
#define Vk_CHECK(ecVar, expr) (ecVar) = (expr); if (Vk_FAILED(ecVar)) return (ecVar);

constexpr VkFormat GetVkFormat(TypeBase type) {
	switch (type) {
		case TypeBase::Bool:           return VK_FORMAT_R8_UINT;
		case TypeBase::UInt32:         return VK_FORMAT_R32_UINT;
		case TypeBase::Int32:          return VK_FORMAT_R32_SINT;
		case TypeBase::UInt64:         return VK_FORMAT_R64_UINT;
		case TypeBase::Int64:          return VK_FORMAT_R64_SINT;

		case TypeBase::UInt32Vector2:  return VK_FORMAT_R32G32_UINT;
		case TypeBase::UInt32Vector3:  return VK_FORMAT_R32G32B32_UINT;
		case TypeBase::UInt32Vector4:  return VK_FORMAT_R32G32B32A32_UINT;

		case TypeBase::Int32Vector2:   return VK_FORMAT_R32G32_SINT;
		case TypeBase::Int32Vector3:   return VK_FORMAT_R32G32B32_SINT;
		case TypeBase::Int32Vector4:   return VK_FORMAT_R32G32B32A32_SINT;

		case TypeBase::UInt64Vector2:  return VK_FORMAT_R64G64_UINT;
		case TypeBase::UInt64Vector3:  return VK_FORMAT_R64G64B64_UINT;
		case TypeBase::UInt64Vector4:  return VK_FORMAT_R64G64B64A64_UINT;

		case TypeBase::Int64Vector2:   return VK_FORMAT_R64G64_SINT;
		case TypeBase::Int64Vector3:   return VK_FORMAT_R64G64B64_SINT;
		case TypeBase::Int64Vector4:   return VK_FORMAT_R64G64B64A64_SINT;

		case TypeBase::Float:          return VK_FORMAT_R32_SFLOAT;
		case TypeBase::FloatVector2:   return VK_FORMAT_R32G32_SFLOAT;
		case TypeBase::FloatVector3:   return VK_FORMAT_R32G32B32_SFLOAT;
		case TypeBase::FloatVector4:   return VK_FORMAT_R32G32B32A32_SFLOAT;

		case TypeBase::FloatMatrix3x3: // Typically 3x vec3, not a single VkFormat
		case TypeBase::FloatMatrix4x4: // Typically 4x vec4, likewise
			return VK_FORMAT_UNDEFINED;

		case TypeBase::Double:         return VK_FORMAT_R64_SFLOAT;
		case TypeBase::DoubleVector2:  return VK_FORMAT_R64G64_SFLOAT;
		case TypeBase::DoubleVector3:  return VK_FORMAT_R64G64B64_SFLOAT;
		case TypeBase::DoubleVector4:  return VK_FORMAT_R64G64B64A64_SFLOAT;

		case TypeBase::DoubleMatrix3x3:
		case TypeBase::DoubleMatrix4x4:
			return VK_FORMAT_UNDEFINED;

		default:
			return VK_FORMAT_UNDEFINED;
	}
}

struct BoundedInstanceData
{
	~BoundedInstanceData() {
		instances.clear();
		//reinterpret_cast<Arena*>(&instances.get_allocator())->destroyAll();
	}
	BoundedInstanceData() = default;
	BoundedInstanceData(const BoundedInstanceData& other) :
		instances{ other.instances.get_allocator() } {
		instances = std::move(other.instances);
	}
	BoundedInstanceData(Arena* arena) :
		instances{ ArenaAllocator<InstanceData>{arena} } {}
	ArenaVector<InstanceData> instances;
	AABB bounds;
};

struct SceneInstancePair
{
	uint16_t sceneIndex;
	uint32_t instanceIndex;

	bool operator==(const SceneInstancePair& rhs) const {
		return sceneIndex == rhs.sceneIndex && instanceIndex == rhs.instanceIndex;
	}

	struct Hash
	{
		size_t operator()(const SceneInstancePair& p) const {
			size_t seed = 0;
			hash_combine(seed, p.sceneIndex);
			hash_combine(seed, p.instanceIndex);
			return seed;
		}
	};
};

struct DescriptorCaps
{
	uint32_t maxSetUBOs = 0;
	uint32_t maxSetSSBOs = 0;
	uint32_t maxSetSampledImages = 0;
	uint32_t maxSetSamplers = 0;

	//update-after-bind:
	uint32_t maxUAB_SampledImages = 0;
	uint32_t maxUAB_UniformBuffers = 0;
	uint32_t maxUAB_StorageBuffers = 0;
	uint32_t maxUAB_SamplerBuffers = 0;
	bool     hasIndexing = false;
};


struct PoolSpec
{
	VkDescriptorPoolCreateFlags flags{};
	std::vector<VkDescriptorPoolSize> sizes;
	uint32_t maxSets{};
};

struct BoundDescriptorKey
{
	VkDescriptorSetLayout layout;
	std::vector<ResourceBinding> bindings;

	bool operator==(const BoundDescriptorKey&) const = default;

	static BoundDescriptorKey nullDescriptor() {
		BoundDescriptorKey key{};

		key.layout = VK_NULL_HANDLE;

		return key;
	}
};

struct BoundDescriptorKeyHash
{
	size_t operator()(const BoundDescriptorKey& key) const {
		size_t seed = reinterpret_cast<uint64_t>(key.layout);
		for (auto b : key.bindings) b.hash(seed);
		return seed;
	}
};

struct PendingWrite
{
	VkWriteDescriptorSet write{};
	VkDescriptorBufferInfo bufferInfo{};
	VkDescriptorImageInfo imageInfo{};
};


class SamplerStatesPresets
{
public:
	static inline VkSamplerCreateInfo point = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,				
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	static inline VkSamplerCreateInfo linear = {
	.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	.magFilter = VK_FILTER_LINEAR,
	.minFilter = VK_FILTER_LINEAR,
	.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.mipLodBias = 0.0f,
	.anisotropyEnable = VK_FALSE,
	.minLod = 0.0f,
	.maxLod = VK_LOD_CLAMP_NONE,
	.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
	.unnormalizedCoordinates = VK_FALSE
	};

	static inline VkSamplerCreateInfo aniso = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 8.0f,
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
};

class VkBlendModes
{
public:
	static inline const VkPipelineColorBlendAttachmentState BlendOpaque = {
	.blendEnable = VK_FALSE,
	.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
	.colorBlendOp = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	.alphaBlendOp = VK_BLEND_OP_ADD,
	.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
						   VK_COLOR_COMPONENT_G_BIT |
						   VK_COLOR_COMPONENT_B_BIT |
						   VK_COLOR_COMPONENT_A_BIT,
	};

	static inline const VkPipelineColorBlendAttachmentState BlendAlpha = {
	.blendEnable = VK_TRUE,
	.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
	.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	.colorBlendOp = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	.alphaBlendOp = VK_BLEND_OP_ADD,
	.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
						   VK_COLOR_COMPONENT_G_BIT |
						   VK_COLOR_COMPONENT_B_BIT |
						   VK_COLOR_COMPONENT_A_BIT,
	};

	static inline const VkPipelineColorBlendAttachmentState BlendPremultiplied = {
	.blendEnable = VK_TRUE,
	.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	.colorBlendOp = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	.alphaBlendOp = VK_BLEND_OP_ADD,
	.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
						   VK_COLOR_COMPONENT_G_BIT |
						   VK_COLOR_COMPONENT_B_BIT |
						   VK_COLOR_COMPONENT_A_BIT,
	};

	static inline const VkPipelineColorBlendAttachmentState BlendAdditive = {
	.blendEnable = VK_TRUE,
	.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
	.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
	.colorBlendOp = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	.alphaBlendOp = VK_BLEND_OP_ADD,
	.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
						   VK_COLOR_COMPONENT_G_BIT |
						   VK_COLOR_COMPONENT_B_BIT |
						   VK_COLOR_COMPONENT_A_BIT,
	};
};

static inline VkPipelineColorBlendAttachmentState GetBlendPreset(BlendMode mode) {
	switch (mode) {
		case BlendMode::Opaque:         return VkBlendModes::BlendOpaque;
		case BlendMode::Alpha:          return VkBlendModes::BlendAlpha;
		case BlendMode::Premultiplied:  return VkBlendModes::BlendPremultiplied;
		case BlendMode::Additive:       return VkBlendModes::BlendAdditive;
		default: return VkBlendModes::BlendOpaque;
	}
}

class VkDepthStates
{
public:
	static inline const VkPipelineDepthStencilStateCreateInfo DepthDefault = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_TRUE,
	.depthWriteEnable = VK_TRUE,
	.depthCompareOp = VK_COMPARE_OP_LESS,
	.depthBoundsTestEnable = VK_FALSE,
	.stencilTestEnable = VK_FALSE,
	.front = {}, // defaults
	.back = {}, // defaults
	.minDepthBounds = 0.0f,
	.maxDepthBounds = 1.0f,
	};

	static inline const VkPipelineDepthStencilStateCreateInfo DepthTestNoWrite = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_TRUE,
	.depthWriteEnable = VK_FALSE,
	.depthCompareOp = VK_COMPARE_OP_LESS
	};

	static inline const VkPipelineDepthStencilStateCreateInfo DepthNone = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_FALSE,
	.depthWriteEnable = VK_FALSE,
	.depthCompareOp = VK_COMPARE_OP_ALWAYS,
	.depthBoundsTestEnable = VK_FALSE,
	.stencilTestEnable = VK_FALSE,
	};
};

static inline VkPipelineDepthStencilStateCreateInfo GetDepthStencilPreset(DepthMode mode) {
	switch (mode) {
		case DepthMode::DepthDefault:
			return VkDepthStates::DepthDefault;
		case DepthMode::None:
			return VkDepthStates::DepthNone;
		case DepthMode::ReadOnly:
			return VkDepthStates::DepthTestNoWrite;

		default: return VkDepthStates::DepthDefault;
	}
}

class VkRasterStates
{
public:
	static inline const VkPipelineRasterizationStateCreateInfo RasterDefault = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	.depthClampEnable = VK_FALSE,
	.rasterizerDiscardEnable = VK_FALSE,
	.polygonMode = VK_POLYGON_MODE_FILL,
	.cullMode = VK_CULL_MODE_BACK_BIT,
	.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	.depthBiasEnable = VK_FALSE,
	.depthBiasConstantFactor = 0.0f,
	.depthBiasClamp = 0.0f,
	.depthBiasSlopeFactor = 0.0f,
	.lineWidth = 1.0f,
	};

	static inline const VkPipelineRasterizationStateCreateInfo RasterWireframe = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	.polygonMode = VK_POLYGON_MODE_LINE,
	.cullMode = VK_CULL_MODE_NONE,
	.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	.lineWidth = 1.0f
	};

	static inline const VkPipelineRasterizationStateCreateInfo Raster_NoCull = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	.polygonMode = VK_POLYGON_MODE_FILL,
	.cullMode = VK_CULL_MODE_NONE,
	.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	.lineWidth = 1.0f
	};
};

static inline VkPipelineRasterizationStateCreateInfo GetRasterPreset(RasterMode mode) {
	switch (mode) {
		case RasterMode::RasterDefault:
			return VkRasterStates::RasterDefault;
		case RasterMode::NoCulling:
			return VkRasterStates::Raster_NoCull;
		case RasterMode::WireFrame:
			return VkRasterStates::RasterWireframe;

		default: return VkRasterStates::RasterDefault;
	}
}

class VkMultiSamplingStates
{
public:
	static inline const VkPipelineMultisampleStateCreateInfo MSAA_1x = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	.sampleShadingEnable = VK_FALSE,
	.minSampleShading = 1.0f,
	.pSampleMask = nullptr,
	.alphaToCoverageEnable = VK_FALSE,
	.alphaToOneEnable = VK_FALSE
	};

	static inline const VkPipelineMultisampleStateCreateInfo MSAA_4x = {
	.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT,
	.sampleShadingEnable = VK_TRUE,
	.minSampleShading = 0.2f,
	.pSampleMask = nullptr,
	.alphaToCoverageEnable = VK_FALSE,
	.alphaToOneEnable = VK_FALSE
	};
};

static inline VkPipelineMultisampleStateCreateInfo GetMultisamplingPreset(MultiSamplingMode mode) {
	switch (mode) {
		default:
			return VkMultiSamplingStates::MSAA_1x;
		case MultiSamplingMode::MSAA_4x:
			return VkMultiSamplingStates::MSAA_4x;
	}
}

class SceneBase;
class VulkanContext
{
public:
	//std::vector<DrawCommand> drawCommands;
	struct DrawContext
	{
		float clearColor[4] = {0.01f, 0.01f, 0.12f, 1.0f};
		float vPortPos[2] = {0,0};
		float vPortSize[2] = {-1, -1};
		float vPortMinDepth = 0.0f;
		float vPortMaxDepth = 1.0f;
		int sciOffset[2] = {0,0};
		int sciSize[2] = {-1, -1};
		
		uint32_t frameCount;
	};
	struct FrameSync
	{
		VkSemaphore imageAvailable;
		VkFence inFlight;
		VkCommandBuffer cmdBuffer;
	};

private:
	uint8_t c_dummyPixel[4] = { 255, 255, 255, 255 };
	bool m_pendingExit = false;
	uint32_t m_lastDrawcallCount = 0;
	uint32_t m_lastPipelineSwitches = 0;
	std::thread m_renderThread;
	FrameArena*  m_mapArenas[VULKAN_FRAMES_IN_FLIGHT];
	std::vector<std::vector<MeshDrawCommand>> drawCmds[VULKAN_FRAMES_IN_FLIGHT];
	std::vector<robin_hood::unordered_flat_set<uint32_t>> submeshKeysWithMultipleInstances[VULKAN_FRAMES_IN_FLIGHT];
	std::vector<robin_hood::unordered_flat_map<uint32_t, BoundedInstanceData>> submeshDrawInstanceData[VULKAN_FRAMES_IN_FLIGHT];
	

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	INLINE MeshDrawCommand subMeshEntity_to_drawCommand(SceneBase* scene, ArenaRegistry& reg, entt::entity entity);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
		}

		return indices;
	};



#ifdef VULKAN_VALIDATION
	static inline const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	inline bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}
#endif

	/*VkDescriptorPool* _checkDefaultBinding(size_t set, const BoundDescriptorKey& key, size_t& outIndex) {
		if (set >= VULKAN_GLOBAL_DESCRIPTOR_SETS)
			return nullptr;

		switch (set) {

			case 0:
			{
				for (auto& resource : key.bindings) {
					switch (resource.binding) {
						default: return nullptr;

						case MAT_BASE_MAT_INSTANCES_INDICES_BIND:
							outIndex = DESCPOOL_INSTANCE_INDEX;
							return &descriptorPoolsDefault[DESCPOOL_INSTANCE_INDEX];

						case MAT_MODELMATRICES_BIND:
						case MAT_CAMERADATA_BIND:
							outIndex = DESCPOOL_BASE_INDEX;
							return &descriptorPoolsDefault[DESCPOOL_BASE_INDEX];

					} break;
				}
			} break;

			case 1:
			{
				for (auto& resource : key.bindings) {
					switch (resource.binding) {
						default: return nullptr;

						case MAT_SAMPLER_BIND:
							outIndex = DESCPOOL_BASE_INDEX;
							return &descriptorPoolsDefault[DESCPOOL_BASE_INDEX];

						case MAT_BASE_MAT_INSTANCES_BIND:
							outIndex = DESCPOOL_INSTANCE_INDEX;
							return &descriptorPoolsDefault[DESCPOOL_INSTANCE_INDEX];

						case MAT_TEXTURES_BIND:
							outIndex = DESCPOOL_TEXTURES_INDEX;
							return &descriptorPoolsDefault[DESCPOOL_TEXTURES_INDEX];
					} break;
				}
			} break;
		}
	}*/

	VkDescriptorPool _makePool(VkDevice device, const PoolSpec& spec) {
		VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		ci.flags = spec.flags;
		ci.maxSets = spec.maxSets;
		ci.poolSizeCount = (uint32_t)spec.sizes.size();
		ci.pPoolSizes = spec.sizes.data();

		VkDescriptorPool pool{};
		VkResult r = vkCreateDescriptorPool(device, &ci, nullptr, &pool);
		if (r != VK_SUCCESS) throw std::runtime_error("vkCreateDescriptorPool failed");
		return pool;
	}


	VkResult _initDescLayoutAndSets(RenderManager* const renderMan, Material& material, VkResult& vkResult,
									std::vector<VkDescriptorSetLayout>& layouts, std::vector<BindSetCombo>& descriptorSetsList,
									bool doAllocs) {

		auto caps = queryDescriptorCaps(m_phyDevice);

		for (auto& [set, key] : material.descriptorSetLayoutKeys) {

			VkDescriptorSetLayout layout;
			auto layoutIt = descSetLayoutCache.find(key);
			std::vector<VkDescriptorSetLayoutBindingFlagsCreateInfo> bindingFlagsList;
			bindingFlagsList.reserve(32);
			if (layoutIt == descSetLayoutCache.end()) {

				// not found, create the descriptor set
				std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setBindings;
				for (size_t i = 0; i < key.bindings.size(); ++i) {
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = key.bindings[i];
					binding.descriptorType = key.types[i];
					binding.descriptorCount = key.counts[i];
					binding.stageFlags = key.stageFlags[i];
					binding.pImmutableSamplers = nullptr;

					setBindings[set].push_back(binding);
				}

				VkDescriptorSetLayoutBindingFlagsCreateInfo* flagsPtr = nullptr;
				//if (set == 0 && material.vShaderName != SHADER_SHAPERENDERER_VS ) {
				//	std::array<VkDescriptorBindingFlags, 3> bindingFlags = {
				//		0,

				//		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, //|
				//		//VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
				//		//VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,

				//		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				//		//VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
				//		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
				//	};
				//	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
				//		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				//		nullptr,
				//		(uint32_t)bindingFlags.size(),
				//		bindingFlags.data()
				//	};
				//	bindingFlagsList.push_back(flagsInfo);
				//	flagsPtr = &bindingFlagsList.back();
				//}

				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.bindingCount = static_cast<uint32_t>(setBindings[set].size());
				layoutInfo.pBindings = setBindings[set].data();
				layoutInfo.pNext = flagsPtr;
				layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

				if (doAllocs)
					Vk_CHECK(vkResult, vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &layout));
				descSetLayoutCache.insert({ key, layout });
			} else
				layout = layoutIt->second;

			uint64_t layoutHandle = reinterpret_cast<uint64_t>(layout);

			std::vector<PendingWrite> pendingWrites;
			pendingWrites.reserve(key.bindings.size() * VULKAN_FRAMES_IN_FLIGHT * 16);

			std::vector< VkDescriptorSetVariableDescriptorCountAllocateInfo> pendingCounts;
			pendingCounts.reserve(key.bindings.size() * VULKAN_FRAMES_IN_FLIGHT);
			std::array<VkDescriptorSet, VULKAN_FRAMES_IN_FLIGHT> descriptorSet;
			BoundDescriptorKey descSetKey{};
			descSetKey.layout = layout;
			for (size_t i = 0; i < key.bindings.size(); ++i) {
				ResourceBinding rBind{};
				rBind.binding = key.bindings[i];
				rBind.type = key.types[i];

				if (set == MAT_CAMERADATA_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_CAMERADATA_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(&camDataBuffer);
					layoutHandle = 0;
					for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {
						PendingWrite pw{};

						pw.bufferInfo.buffer = camDataBuffer[j];
						pw.bufferInfo.offset = 0;
						pw.bufferInfo.range = sizeof(CameraData);

						pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
						pw.write.dstBinding = key.bindings[i];
						pw.write.dstArrayElement = 0;
						pw.write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						pw.write.descriptorCount = 1;

						pendingWrites.push_back(pw);
						pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;
					}

				} else if (set == MAT_TEXTURES_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_TEXTURES_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(&texResources);
					layoutHandle = 0;

					VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
					uint32_t textures = 0;
					countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
					countInfo.descriptorSetCount = 1;
					countInfo.pDescriptorCounts = &textures;
					pendingCounts.push_back(countInfo);

				} else if (set == MAT_BASE_MAT_INSTANCES_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_BASE_MAT_INSTANCES_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(&matBuf_baseMatInstances);
					layoutHandle = 0;



				} else if (set == MAT_MODELMATRICES_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_MODELMATRICES_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(&matBuf_modelTransforms);
					layoutHandle = 0;
					for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

						PendingWrite pw{};

						pw.bufferInfo.buffer = matBuf_modelTransforms[j];
						pw.bufferInfo.offset = 0;
						pw.bufferInfo.range = sizeof(ModelTransform);

						pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
						pw.write.dstBinding = key.bindings[i];
						pw.write.dstArrayElement = 0;
						pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						pw.write.descriptorCount = 1; // caps.maxSetSSBOs;

						pendingWrites.push_back(pw);
						pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;
					}

				} else if (set == MAT_SAMPLER_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_SAMPLER_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(&baseSamplers);
					layoutHandle = 0;

				} else if (set == MAT_BASE_INSTANCES_DATA_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_BASE_INSTANCES_DATA_NAME) {

					rBind.handle = reinterpret_cast<uint64_t>(this);
					layoutHandle = 0;
					for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

						PendingWrite pw{};

						pw.bufferInfo.buffer = instanceIndexBuffer[j];
						pw.bufferInfo.offset = 0;
						pw.bufferInfo.range = VK_WHOLE_SIZE;

						pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
						pw.write.dstBinding = key.bindings[i];
						pw.write.dstArrayElement = 0;
						pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						pw.write.descriptorCount = 1;// caps.maxSetSSBOs;

						pendingWrites.push_back(pw);
						pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;

					}
				} else if (set == MAT_GPULIGHTS_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_GPULIGHTS_NAME) {

					rBind.handle = reinterpret_cast<uint64_t>(this);
					layoutHandle = 0;

					//for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

					//	PendingWrite pw{};

					//	pw.bufferInfo.buffer = lightsBuffer[j];
					//	pw.bufferInfo.offset = 0;
					//	pw.bufferInfo.range = VK_WHOLE_SIZE;

					//	pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					//	pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
					//	pw.write.dstBinding = key.bindings[i];
					//	pw.write.dstArrayElement = 0;
					//	pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					//	pw.write.descriptorCount = 1;

					//	pendingWrites.push_back(pw);
					//	pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;
					//}

					
				} else if (set == MAT_CLUSTER_LIGHTCOUNT_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_CLUSTER_LIGHTCOUNT_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(this);
					layoutHandle = 0;

					for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

						PendingWrite pw{};

						pw.bufferInfo.buffer = lightsClusterCountBuffer[j];
						pw.bufferInfo.offset = 0;
						pw.bufferInfo.range = VK_WHOLE_SIZE;

						pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
						pw.write.dstBinding = key.bindings[i];
						pw.write.dstArrayElement = 0;
						pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						pw.write.descriptorCount = 1;

						pendingWrites.push_back(pw);
						pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;
					}
				} else if (set == MAT_CLUSTER_LIGHTINDICES_SET && renderMan->getParamName(key.nameIndices[i]) == MAT_CLUSTER_LIGHTINDICES_NAME) {
					rBind.handle = reinterpret_cast<uint64_t>(this);
					layoutHandle = 0;

					for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

						PendingWrite pw{};

						pw.bufferInfo.buffer = lightsClusterIndicesBuffer[j];
						pw.bufferInfo.offset = 0;
						pw.bufferInfo.range = VK_WHOLE_SIZE;

						pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						pw.write.dstSet = reinterpret_cast<VkDescriptorSet>(j); //hacky workaround for init
						pw.write.dstBinding = key.bindings[i];
						pw.write.dstArrayElement = 0;
						pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						pw.write.descriptorCount = 1;

						pendingWrites.push_back(pw);
						pendingWrites.back().write.pBufferInfo = &pendingWrites.back().bufferInfo;
					}
				}

				descSetKey.bindings.push_back(rBind);
			}
			auto descSetIt = descriptorSetCache.find(descSetKey);
			if (descSetIt == descriptorSetCache.end()) {

				// TODO TODO
				void* allocPtr = nullptr;
				if (!pendingCounts.empty()) {
					allocPtr = &pendingCounts[0];

					for (size_t i = 0; i < pendingCounts.size() - 1; ++i) {
						pendingCounts[i].pNext = &pendingCounts[i + 1];
					}
				}

				for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; ++j) {

					VkDescriptorPool descPool{};
					if (set >= VULKAN_GLOBAL_DESCRIPTOR_SETS)
						descPool = descriptorPoolsDynamic.back();
					else
						descPool = set == 0 ? descriptorPoolsDefault[0] : descriptorPoolsDefault[1];
						

					// allocate and update descriptor set
					if (doAllocs) {
						VkDescriptorSetAllocateInfo allocInfo{};
						allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
						allocInfo.descriptorPool = descPool;
						allocInfo.descriptorSetCount = 1;
						allocInfo.pSetLayouts = &layout;
						allocInfo.pNext = allocPtr;

						auto allocResult = vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &descriptorSet[j]);
						if (allocResult == VK_ERROR_OUT_OF_POOL_MEMORY || allocResult == VK_ERROR_FRAGMENTED_POOL) {
							if (set >= VULKAN_GLOBAL_DESCRIPTOR_SETS)
								throw std::exception("BASE DESCRIPTOR POOLS TOO SMALL!");

							descriptorPoolsDynamic.emplace_back(_makePool(m_vkDevice,
																		  PoolSpec{
																			  /*flags*/ VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
																			  /*sizes*/{
																				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VULKAN_DESCPOOL_DYNAMIC_UNIFORMS },
																				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VULKAN_DESCPOOL_DYNAMIC_STORAGE },
																				{ VK_DESCRIPTOR_TYPE_SAMPLER,        VULKAN_DESCPOOL_DYNAMIC_SAMPLER  },
																				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VULKAN_DESCPOOL_DYNAMIC_IMAGE }
																			  },
																		  /*maxSets*/ 256 }
							));
							allocInfo.descriptorPool = descriptorPoolsDynamic.back();
							Vk_CHECK(vkResult, vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &descriptorSet[j]));
						}
					}
				}

				for (auto& pw : pendingWrites) {
					pw.write.dstSet = descriptorSet[reinterpret_cast<size_t>(pw.write.dstSet)]; // correcting back the hackywacky
					vkUpdateDescriptorSets(m_vkDevice, 1, &pw.write, 0, nullptr);
				}

				descriptorSetCache.insert({ descSetKey, descriptorSet });
			} else
				descriptorSet = descSetIt->second;

			// for later mapping
			for (auto& binding : key.bindings) {
				if (material.name() == "ShapeRendererMaterial") {
					for (size_t k = 0; k < VULKAN_FRAMES_IN_FLIGHT; ++k) {
						shapeRendererDescSet[k] = descriptorSet[k];
					}
					break;;
				}

				BindSetCombo combo{ static_cast<uint8_t>(binding), static_cast<uint8_t>(set), layoutHandle };
				auto insertResult = bindingToDescriptorSet.insert({ combo, descriptorSet });
				descriptorSetsList.push_back(combo);
			}

			layouts.resize(std::max(layouts.size(), static_cast<size_t>(set + 1)));
			layouts[set] = layout;
		}

		// Dummy texture
		if (!dummyTexCreated) {
			dummyTexCreated = true;

			Texture2D dummyTex{};
			dummyTex.width = 1;
			dummyTex.height = 1;
			dummyTex.texelPtr = reinterpret_cast<void*>(&c_dummyPixel[0]);
			dummyTex = renderMan->registerTexture("DummyTex", dummyTex);
			uint32_t texIndex{};
			loadTexture(dummyTex, &texIndex);
			texResources[0].imageInfo.sampler = baseSamplers[{SamplerMode::Aniso8X, SamplerAddressMode::Repeat}];

			std::vector<VkDescriptorImageInfo> imageInfos;
			imageInfos.reserve(VULKAN_FRAMES_IN_FLIGHT * 3);
			for (size_t j = 0; j < VULKAN_FRAMES_IN_FLIGHT; j++) {

				
				for (int k = 0; k < 1; ++k) {
					VkDescriptorImageInfo info{};
					info.sampler = texResources[0].imageInfo.sampler;
					info.imageView = texResources[0].imageView;
					info.imageLayout = texResources[0].imageInfo.imageLayout;
					imageInfos.push_back(info);
				}

				VkDescriptorSet set = bindingToDescriptorSet[{MAT_SAMPLER_BIND, MAT_SAMPLER_SET, 0}][j];

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = MAT_SAMPLER_BIND;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = imageInfos.data();

				vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
			}
		}

		return VK_SUCCESS;
	}



public:
	WindowSurface* const windowSurface;
	VkInstance m_vkInstance = nullptr;
	VkSurfaceKHR m_vkSurface = nullptr;
	VkPhysicalDevice m_phyDevice = nullptr;
	VkDevice m_vkDevice = nullptr;
	VkQueue m_graphicsQueue = nullptr;
	uint32_t m_graphicsQueueFamilyIndex = static_cast<uint32_t>(-1);
	DescriptorCaps m_currentCaps{};

	std::unordered_map<SamplerState, VkSampler, SamplerStateHash> baseSamplers;

	// Descriptor Layouts & Sets
	std::unordered_map<DescriptorSetLayoutKey, VkDescriptorSetLayout, DescriptorSetLayoutKeyHash> descSetLayoutCache;
	std::unordered_map<BoundDescriptorKey, std::array<VkDescriptorSet, VULKAN_FRAMES_IN_FLIGHT>, BoundDescriptorKeyHash> descriptorSetCache;

	std::unordered_map<BindSetCombo, std::array<VkDescriptorSet, VULKAN_FRAMES_IN_FLIGHT>, BindSetComboKeyHash> bindingToDescriptorSet;

	std::unordered_map<PipelineKey, uint32_t, PipelineKeyHasher> pipelineHashIndexMap;
	std::vector<VkPipeline> pipelines;
	std::vector<VkPipelineLayout> pipelineLayouts;

	//BlendMode currentBlendMode = BlendMode::Opaque;
	Arena* instanceDataArena[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	uint32_t renderPassIndex = 0;
	uint32_t pipelineId = 0;
	bool isClean = true;
	uint8_t currentFrame = 0;
	bool pendingResize = false;
	bool dummyTexCreated = false;

	VkPresentModeKHR presentMode;
	VkSwapchainKHR swapchain = nullptr;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImage> depthStencilImages;
	std::vector<VkImageView> depthStencilViews;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkDeviceMemory> depthStencilMemories;
	VkSurfaceFormatKHR surfaceFormat;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	std::vector<VkDescriptorPool> descPools;


	///// DESCRIPTOR POOLS //////////////////////////////////////////////////
	std::array<VkDescriptorPool, 2> descriptorPoolsDefault{};
	std::vector<VkDescriptorPool> descriptorPoolsDynamic;

	VkCommandPool commandPool = nullptr;
	
	std::vector<VkRenderPass> rendPasses;
	std::vector<VkFramebuffer> swapChainFramebuffers;	
	std::array<FrameSync, VULKAN_FRAMES_IN_FLIGHT> frameSync;
	std::vector<VkSemaphore> imageRenderFinished;

	// Shape Rendering
	VkDescriptorSet shapeRendererDescSet[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkBuffer shapeVertexBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkBuffer shapeIndexBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory shapeVertexMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory shapeIndexMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };


	// Staging buffer
	size_t matStageCurrentSize[VULKAN_FRAMES_IN_FLIGHT];// = VULKAN_MATSTAGEBUF_SIZE;
	void* matStagingPtr[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkBuffer matStagingBuf[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory matStagingMem[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };

	// Global Material Buffers
	std::vector<BaseVSIn> vertices;
	std::vector<uint32_t> indices;
	std::array<VkVertexInputAttributeDescription, 5> vertexAttributes{};
	VkBuffer vertexBuffer = nullptr;;
	VkDeviceMemory vertexMemory = nullptr;;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexMemory = nullptr;

	CameraData camData{};
	VkBuffer camDataBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory camDataMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };

	std::vector<ModelTransform> moddelTransforms;
	VkBuffer matBuf_modelTransforms[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory matDevMem_modelTransforms[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	size_t modelTransforms_lastBufferSize[VULKAN_FRAMES_IN_FLIGHT] = { 0, 0 };

	std::vector<VkTextureResource> texResources;

	//std::vector<uint32_t> instanceIndexStage;
	VkBuffer instanceIndexBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory instanceIndexMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	size_t instancesIndex_lastBufferSize[VULKAN_FRAMES_IN_FLIGHT] = { 0, 0 };

	std::vector<BaseMaterialInstance> matData_baseMatInstances;
	VkBuffer matBuf_baseMatInstances[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory matDevMem_baseMatInstances[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };

	std::vector<GPULight> lightsData;
	VkBuffer lightsBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory lightsMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	std::vector<Index32> lightsClusterIndicesData;
	VkBuffer lightsClusterIndicesBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory lightsClusterIndicesMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };

	std::vector<Index128> lightsClusterCountData;
	VkBuffer lightsClusterCountBuffer[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };
	VkDeviceMemory lightsClusterCountMemory[VULKAN_FRAMES_IN_FLIGHT] = { nullptr, nullptr };

	std::vector<GPULight> pendingLightUpdates;

	VkPipelineLayout lightCluster_pipelineLayout;
	VkPipeline lightCluster_pipeline;
	std::array<std::vector<VkDescriptorSet>, VULKAN_FRAMES_IN_FLIGHT> lightCluster_descSets;
	bool lightCluster_pipelineCreated = false;

	uint32_t registerBaseMaterialInstance(const BaseMaterialInstance* const matInstance) {
		auto index = matData_baseMatInstances.size();
		if (matInstance)
			matData_baseMatInstances.push_back(*matInstance);
		else
			matData_baseMatInstances.push_back({});
		return index;
	}

	INLINE BaseMaterialInstance* getBaseMaterialInstanceData(const uint32_t index) {
		if (index >= matData_baseMatInstances.size())
			return nullptr;
		return &matData_baseMatInstances[index];
	}
	

	void setPendingExit() { m_pendingExit = true; }
	const bool& isPendingExit() const { return m_pendingExit; }

	~VulkanContext() {

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			delete instanceDataArena[i];
			instanceDataArena[i] = nullptr;

			delete m_mapArenas[i];
			m_mapArenas[i] = nullptr;

			//delete drawCmds[i];
			//drawCmds[i] = nullptr;
		}

		if (!isClean)
			cleanUp();
		glfwTerminate();
	}
	VulkanContext() = delete;
	inline VulkanContext(WindowSurface* const wndSurface) :
		windowSurface{ wndSurface },
		commandPool{nullptr},
		m_phyDevice{nullptr},
		frameSync{},
		m_graphicsQueue{nullptr},
		m_vkInstance{nullptr},
		m_vkDevice{nullptr},
		m_graphicsQueueFamilyIndex{ static_cast<uint32_t>(-1) },
		m_vkSurface{nullptr},
		swapchain{nullptr},
		swapchainFormat{},
		swapchainExtent{},
		surfaceFormat{},
		presentMode{},
		renderPassIndex{},
		camData{},
		lightCluster_pipeline{nullptr},
		lightCluster_pipelineLayout{nullptr}
	{
		pendingLightUpdates.reserve(256);
		rendPasses.reserve(2048);
		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			instanceDataArena[i] = new Arena{ 128_MB };
			m_mapArenas[i] = new FrameArena{ 32_MB };

			//drawCmds[i] = new FrameArenaVector<MeshDrawCommand>{ FrameArenaAllocator<MeshDrawCommand>{m_mapArenas[i]} };
		}
	}
	INLINE VkResult _appendStageBuffer(uint8_t fit, size_t requestedSize ) {
		VkResult vkResult{};
		if (matStageCurrentSize[fit] >= requestedSize)
			return VK_SUCCESS;



	}

	INLINE VkResult _checkStageRealloc(uint8_t fit, size_t requestedSize) {
		if (matStageCurrentSize[fit] >= requestedSize)
			return VK_SUCCESS;
		VkResult vkResult{};

		auto newSize = requestedSize * 2;
		

		static std::vector<uint8_t> tempBytes[VULKAN_FRAMES_IN_FLIGHT];
		tempBytes[fit].reserve(matStageCurrentSize[fit]);
		tempBytes[fit].clear();
		std::memcpy(static_cast<uint8_t*>(tempBytes[fit].data()),
					static_cast<uint8_t*>(matStagingPtr[fit]),
					matStageCurrentSize[fit]);

		
		vkUnmapMemory(m_vkDevice, matStagingMem[fit]);

		vkDestroyBuffer(m_vkDevice, matStagingBuf[fit], nullptr);

		if (matStagingMem[fit] != VK_NULL_HANDLE)
			vkFreeMemory(m_vkDevice, matStagingMem[fit], nullptr);

		// Staging Buffers
		VkBufferCreateInfo matStagebufferInfo{};
		matStagebufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		matStagebufferInfo.size = newSize;
		matStagebufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		matStagebufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &matStagebufferInfo, nullptr, &matStagingBuf[fit]));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_vkDevice, matStagingBuf[fit], &memRequirements);

		VkMemoryAllocateInfo matStageAllocInfo{};
		matStageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		matStageAllocInfo.allocationSize = memRequirements.size;
		matStageAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
														   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
														   m_phyDevice);

		Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &matStageAllocInfo, nullptr, &matStagingMem[fit]));
		Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, matStagingBuf[fit], matStagingMem[fit], 0));
		vkMapMemory(m_vkDevice, matStagingMem[fit], 0, newSize, 0, &matStagingPtr[fit]);
		
		
		std::memcpy(static_cast<uint8_t*>(matStagingPtr[fit]),
					static_cast<uint8_t*>(tempBytes[fit].data()),
					matStageCurrentSize[fit]);

		matStageCurrentSize[fit] = newSize;

		return VK_SUCCESS;
	}
	

	INLINE void registerNewLight(const GPULight& light) {


		pendingLightUpdates.push_back(light);
	}

	INLINE uint32_t lastDrawcallCount() const { return m_lastDrawcallCount; }
	INLINE uint32_t lastPipelineSwitchCount() const { return m_lastPipelineSwitches; }

	INLINE VkTextureResource* getTexResource(const size_t resourceIndex) {
		if (resourceIndex >= texResources.size())
			return nullptr;
		return &texResources[resourceIndex];
	}
	INLINE std::vector<VkTextureResource>& textureResources() { return texResources; }
	INLINE void registerMesh(BaseVSIn* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount) {
		
		auto vertStartId = this->vertices.size();
		this->vertices.resize(vertStartId + vertexCount);
		std::memcpy(&this->vertices[vertStartId], vertices, vertexCount * sizeof(BaseVSIn));

		auto indexStartId = this->indices.size();
		this->indices.resize(indexStartId + indexCount);
		std::memcpy(&this->indices[indexStartId], indices, indexCount * sizeof(uint32_t));

		
	}

	INLINE VkResult loadBaseMatData() {

		VkResult vkResult{};

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			VkDeviceSize bufferSize = sizeof(BaseMaterialInstance) * this->matData_baseMatInstances.size();
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.size = bufferSize;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &matBuf_baseMatInstances[i]));
			VkMemoryRequirements bufferMemReq{};
			vkGetBufferMemoryRequirements(m_vkDevice, matBuf_baseMatInstances[i], &bufferMemReq);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = bufferMemReq.size;
			allocInfo.memoryTypeIndex = findMemoryType(
				bufferMemReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_phyDevice
			);
			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &matDevMem_baseMatInstances[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, matBuf_baseMatInstances[i], matDevMem_baseMatInstances[i], 0));

			void* data;
			vkMapMemory(m_vkDevice, matDevMem_baseMatInstances[i], 0, bufferSize, 0, &data);
			memcpy(data, this->matData_baseMatInstances.data(), (size_t)bufferSize);
			vkUnmapMemory(m_vkDevice, matDevMem_baseMatInstances[i]);


			// Update descriptor sets
			VkDescriptorSet set = bindingToDescriptorSet[{MAT_BASE_MAT_INSTANCES_BIND, MAT_BASE_MAT_INSTANCES_SET, 0}][i];
			// Only update the buffers that changed
			std::vector<VkWriteDescriptorSet> descriptorWrites;

			// Say only your instance data changed this frame
			VkDescriptorBufferInfo instanceDataBufferInfo{};
			instanceDataBufferInfo.buffer = matBuf_baseMatInstances[i];
			instanceDataBufferInfo.offset = 0;
			instanceDataBufferInfo.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet instanceDataWrite{};
			instanceDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			instanceDataWrite.dstSet = set;
			instanceDataWrite.dstBinding = MAT_BASE_MAT_INSTANCES_BIND;
			instanceDataWrite.dstArrayElement = 0;
			instanceDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			instanceDataWrite.descriptorCount = 1;
			instanceDataWrite.pBufferInfo = &instanceDataBufferInfo;

			// Only update what changed
			vkUpdateDescriptorSets(m_vkDevice, 1, &instanceDataWrite, 0, nullptr);

		}

		return VK_SUCCESS;
	}

	INLINE VkResult updateLightsData() {
		VkResult vkResult{};

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			VkDeviceSize bufferSize = sizeof(GPULight) * this->lightsData.size();
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.size = bufferSize;

			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &lightsBuffer[i]));
			VkMemoryRequirements lightDataBufferMemReq{};
			vkGetBufferMemoryRequirements(m_vkDevice, lightsBuffer[i], &lightDataBufferMemReq);
			VkMemoryAllocateInfo lightDataAllocInfo{};
			lightDataAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			lightDataAllocInfo.allocationSize = lightDataBufferMemReq.size;
			lightDataAllocInfo.memoryTypeIndex = findMemoryType(
				lightDataBufferMemReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_phyDevice
			);
			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &lightDataAllocInfo, nullptr, &lightsMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, lightsBuffer[i], lightsMemory[i], 0));

			// Update descriptor sets
			VkDescriptorSet set = bindingToDescriptorSet[{MAT_GPULIGHTS_BIND, MAT_GPULIGHTS_SET, 0}][i];
			// Only update the buffers that changed
			std::vector<VkWriteDescriptorSet> descriptorWrites;

			// Say only your instance data changed this frame
			VkDescriptorBufferInfo lightsDataBufferInfo{};
			lightsDataBufferInfo.buffer = lightsBuffer[i];
			lightsDataBufferInfo.offset = 0;
			lightsDataBufferInfo.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet instanceDataWrite{};
			instanceDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			instanceDataWrite.dstSet = set;
			instanceDataWrite.dstBinding = MAT_GPULIGHTS_BIND;
			instanceDataWrite.dstArrayElement = 0;
			instanceDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			instanceDataWrite.descriptorCount = 1;
			instanceDataWrite.pBufferInfo = &lightsDataBufferInfo;

			// Only update what changed
			vkUpdateDescriptorSets(m_vkDevice, 1, &instanceDataWrite, 0, nullptr);


			void* mappedData = nullptr;
			vkMapMemory(m_vkDevice, lightsMemory[i], 0, bufferSize, 0, &mappedData);
			memcpy(mappedData, this->lightsData.data(), bufferSize);
			vkUnmapMemory(m_vkDevice, lightsMemory[i]);
		}

		return VK_SUCCESS;
	}

	INLINE VkResult reallocateVertexIndexBuffers() {
		VkResult vkResult{};

		// Vertexbuffer
		bool destroyedVertex = false;
		if (vertexBuffer) {
			vkDestroyBuffer(m_vkDevice, vertexBuffer, nullptr);
			vertexBuffer = VK_NULL_HANDLE;
			destroyedVertex = true;
		}
		if (vertexMemory) {
			vkFreeMemory(m_vkDevice, vertexMemory, nullptr);
			vertexMemory = VK_NULL_HANDLE;
			destroyedVertex = true;
		}
		VkDeviceSize vertexBufferSize = sizeof(BaseVSIn) * this->vertices.size();
		VkBufferCreateInfo vBufferInfo{};
		vBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vBufferInfo.size = vertexBufferSize;
		Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &vBufferInfo, nullptr, &vertexBuffer));
		VkMemoryRequirements vBufferMemReq{};
		vkGetBufferMemoryRequirements(m_vkDevice, vertexBuffer, &vBufferMemReq);
		VkMemoryAllocateInfo vertexAllocInfo{};
		vertexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vertexAllocInfo.allocationSize = vBufferMemReq.size;
		vertexAllocInfo.memoryTypeIndex = findMemoryType(
			vBufferMemReq.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_phyDevice
		);
		Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &vertexAllocInfo, nullptr, &vertexMemory));
		Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, vertexBuffer, vertexMemory, 0));

		void* data;
		vkMapMemory(m_vkDevice, vertexMemory, 0, vertexBufferSize, 0, &data);
		memcpy(data, this->vertices.data(), (size_t)vertexBufferSize);
		vkUnmapMemory(m_vkDevice, vertexMemory);

		// IndexBuffer
		if (indexBuffer) {
			vkDestroyBuffer(m_vkDevice, indexBuffer, nullptr);
			indexBuffer = VK_NULL_HANDLE;
		}
		if (indexMemory) {
			vkFreeMemory(m_vkDevice, indexMemory, nullptr);
			indexMemory = VK_NULL_HANDLE;
		}
		VkDeviceSize indexBufferSize = sizeof(uint32_t) * this->indices.size();
		VkBufferCreateInfo iBufferInfo{};
		iBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		iBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		iBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		iBufferInfo.size = indexBufferSize;
		Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &iBufferInfo, nullptr, &indexBuffer));
		VkMemoryRequirements iBufferMemReq{};
		vkGetBufferMemoryRequirements(m_vkDevice, indexBuffer, &iBufferMemReq);
		VkMemoryAllocateInfo indexAllocInfo{};
		indexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		indexAllocInfo.allocationSize = iBufferMemReq.size;
		indexAllocInfo.memoryTypeIndex = findMemoryType(
			iBufferMemReq.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_phyDevice
		);
		Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &indexAllocInfo, nullptr, &indexMemory));
		Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, indexBuffer, indexMemory, 0));

		//void* data;
		vkMapMemory(m_vkDevice, indexMemory, 0, indexBufferSize, 0, &data);
		memcpy(data, this->indices.data(), (size_t)indexBufferSize);
		vkUnmapMemory(m_vkDevice, indexMemory);


		return VK_SUCCESS;
	}

	INLINE VkResult initVulkan(const VkPresentModeKHR mode, bool prioIGpu = false) {
		VkResult vk{};

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			frameSync[i] = FrameSync{};
		}
		presentMode = mode;
		renderPassIndex = 0;
		//Vk_CHECK(vk, createInstanceGlfw());
		Vk_CHECK(vk, createInstance());
		Vk_CHECK(vk, createSurface());
		Vk_CHECK(vk, pickPhysDevice(prioIGpu));
		Vk_CHECK(vk, createLogicalDevice());
		Vk_CHECK(vk, createSwapchain());
		Vk_CHECK(vk, createImageViews());
		Vk_CHECK(vk, createRenderPass());
		Vk_CHECK(vk, createFramebuffers());
		Vk_CHECK(vk, createCommandPool());
		Vk_CHECK(vk, createCommandBuffer());
		Vk_CHECK(vk, createSyncObjects());
		Vk_CHECK(vk, createSamplerPresets());
		Vk_CHECK(vk, createDescriptorPool());
		Vk_CHECK(vk, setupVertexBuffer());
		Vk_CHECK(vk, createGlobalBuffers());
		//Vk_CHECK(vk, createLightClusterPipeline());

		isClean = false;

		return VK_SUCCESS;
	}

	inline VkResult setupVertexBuffer() {
		VkResult vkResult;

		vertexAttributes[0] = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(BaseVSIn, pos)
		};

		vertexAttributes[1] = {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(BaseVSIn, normal)
		};

		vertexAttributes[2] = {
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(BaseVSIn, tangent)
		};

		vertexAttributes[3] = {
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(BaseVSIn, binormal)
		};

		vertexAttributes[4] = {
			.location = 4,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(BaseVSIn, texCoord)
		};

		VkVertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.stride = sizeof(BaseVSIn);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return VK_SUCCESS;
	}

	INLINE VkResult createLightClusterPipeline(Shader* const lightCsShader) {
		VkResult vkResult{};

		std::vector< VkDescriptorSetLayoutBinding> lightsBindings;
		lightsBindings.push_back({ MAT_GPULIGHTS_BIND, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
					VK_SHADER_STAGE_COMPUTE_BIT, nullptr });
		lightsBindings.push_back({ MAT_CLUSTER_LIGHTCOUNT_BIND, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
					VK_SHADER_STAGE_COMPUTE_BIT, nullptr });
		lightsBindings.push_back({ MAT_CLUSTER_LIGHTINDICES_BIND, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
					VK_SHADER_STAGE_COMPUTE_BIT, nullptr });

		VkDescriptorSetLayoutCreateInfo dslCI_lights{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		dslCI_lights.bindingCount = (uint32_t)lightsBindings.size();
		dslCI_lights.pBindings = lightsBindings.data();

		VkDescriptorSetLayout setLayoutLights;
		Vk_CHECK(vkResult, vkCreateDescriptorSetLayout(m_vkDevice, &dslCI_lights, nullptr, &setLayoutLights));



		VkDescriptorSetLayoutCreateInfo emptyCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		emptyCI.bindingCount = 0;
		emptyCI.pBindings = nullptr;

		VkDescriptorSetLayout emptySet1;
		vkCreateDescriptorSetLayout(m_vkDevice, &emptyCI, nullptr, &emptySet1);



		std::vector<VkDescriptorSetLayoutBinding> camBindings;
		camBindings.push_back({ MAT_CAMERADATA_BIND, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr });

		VkDescriptorSetLayoutCreateInfo dslCI_cam{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		dslCI_cam.bindingCount = (uint32_t)camBindings.size();
		dslCI_cam.pBindings = camBindings.data();

		VkDescriptorSetLayout setLayoutCam;
		Vk_CHECK(vkResult, vkCreateDescriptorSetLayout(m_vkDevice, &dslCI_cam, nullptr, &setLayoutCam));

		std::array<VkDescriptorSetLayout, 3> setLayouts{ setLayoutCam, emptySet1, setLayoutLights };

		VkPipelineLayoutCreateInfo plCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		plCI.setLayoutCount = (uint32_t)setLayouts.size();
		plCI.pSetLayouts = setLayouts.data();

		VkPipelineLayout pipelineLayout;
		Vk_CHECK(vkResult, vkCreatePipelineLayout(m_vkDevice, &plCI, nullptr, &pipelineLayout));
		lightCluster_pipelineLayout = pipelineLayout;




		VkShaderModule csModule{};
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = lightCsShader->bytecode.size() * sizeof(uint32_t);
		createInfo.pCode = lightCsShader->bytecode.data();
		Vk_CHECK(vkResult, vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &csModule));


		VkPipelineShaderStageCreateInfo stageCI{};
		stageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageCI.module = csModule;
		stageCI.pName = lightCsShader->entryPoint.c_str();

		VkComputePipelineCreateInfo cpCI{
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
		};
		cpCI.stage = stageCI;
		cpCI.layout = pipelineLayout;

		VkPipeline computePipeline;
		Vk_CHECK(vkResult, vkCreateComputePipelines(m_vkDevice, VK_NULL_HANDLE, 1, &cpCI, nullptr, &computePipeline));
		lightCluster_pipeline = computePipeline;

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorSetAllocateInfo allocInfo1{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo1.descriptorPool = descriptorPoolsDynamic.back();
			allocInfo1.descriptorSetCount = 1;
			allocInfo1.pSetLayouts = &setLayouts[0];
			VkDescriptorSet descSet1;
			Vk_CHECK(vkResult, vkAllocateDescriptorSets(m_vkDevice, &allocInfo1, &descSet1));

			VkDescriptorSetAllocateInfo allocInfo2{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo2.descriptorPool = descriptorPoolsDynamic.back();
			allocInfo2.descriptorSetCount = 1;
			allocInfo2.pSetLayouts = &setLayouts[1];
			VkDescriptorSet descSet2;
			Vk_CHECK(vkResult, vkAllocateDescriptorSets(m_vkDevice, &allocInfo2, &descSet2));

			VkDescriptorSetAllocateInfo allocInfo3{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo3.descriptorPool = descriptorPoolsDynamic.back();
			allocInfo3.descriptorSetCount = 1;
			allocInfo3.pSetLayouts = &setLayouts[2];
			VkDescriptorSet descSet3;
			Vk_CHECK(vkResult, vkAllocateDescriptorSets(m_vkDevice, &allocInfo3, &descSet3));



			lightCluster_descSets[i].push_back(descSet1);
			lightCluster_descSets[i].push_back(descSet2);
			lightCluster_descSets[i].push_back(descSet3);



			VkDescriptorBufferInfo buffInfo1{};
			buffInfo1.buffer = camDataBuffer[i];
			buffInfo1.offset = 0;
			buffInfo1.range = sizeof(CameraData);

			VkWriteDescriptorSet write1{};
			write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write1.dstSet = lightCluster_descSets[i][0];
			write1.dstBinding = MAT_CAMERADATA_BIND;
			write1.dstArrayElement = 0;
			write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write1.descriptorCount = 1;
			write1.pNext = nullptr;
			write1.pBufferInfo = &buffInfo1;



			VkDescriptorBufferInfo buffInfo2{};
			buffInfo2.buffer = lightsBuffer[i];
			buffInfo2.offset = 0;
			buffInfo2.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet write2{};
			write2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write2.dstSet = lightCluster_descSets[i][2];
			write2.dstBinding = MAT_GPULIGHTS_BIND;
			write2.dstArrayElement = 0;
			write2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write2.descriptorCount = 1;
			write2.pNext = nullptr;
			write2.pBufferInfo = &buffInfo2;



			VkDescriptorBufferInfo buffInfo3{};
			buffInfo3.buffer = lightsClusterCountBuffer[i];
			buffInfo3.offset = 0;
			buffInfo3.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet write3{};
			write3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write3.dstSet = lightCluster_descSets[i][2];
			write3.dstBinding = MAT_CLUSTER_LIGHTCOUNT_BIND;
			write3.dstArrayElement = 0;
			write3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write3.descriptorCount = 1;
			write3.pNext = nullptr;
			write3.pBufferInfo = &buffInfo3;



			VkDescriptorBufferInfo buffInfo4{};
			buffInfo4.buffer = lightsClusterIndicesBuffer[i];
			buffInfo4.offset = 0;
			buffInfo4.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet write4{};
			write4.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write4.dstSet = lightCluster_descSets[i][2];
			write4.dstBinding = MAT_CLUSTER_LIGHTINDICES_BIND;
			write4.dstArrayElement = 0;
			write4.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write4.descriptorCount = 1;
			write4.pNext = nullptr;
			write4.pBufferInfo = &buffInfo4;

			std::vector<VkWriteDescriptorSet> writes{ write1, write2, write3, write4 };

			vkUpdateDescriptorSets(m_vkDevice, (uint32_t)writes.size(), writes.data(), 0, nullptr);
		}

		lightCluster_pipelineCreated = true;

		return VK_SUCCESS;
	}

	INLINE VkResult createGlobalBuffers() {
		VkResult vkResult;
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating and binding global buffers... ");

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {

			// Staging Buffers
			matStageCurrentSize[i] = VULKAN_MATSTAGEBUF_SIZE;

			VkBufferCreateInfo matStagebufferInfo{};
			matStagebufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			matStagebufferInfo.size = matStageCurrentSize[i];
			matStagebufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			matStagebufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &matStagebufferInfo, nullptr, &matStagingBuf[i]));

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(m_vkDevice, matStagingBuf[i], &memRequirements);

			VkMemoryAllocateInfo matStageAllocInfo{};
			matStageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			matStageAllocInfo.allocationSize = memRequirements.size;
			matStageAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
																 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																 m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &matStageAllocInfo, nullptr, &matStagingMem[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, matStagingBuf[i], matStagingMem[i], 0));
			vkMapMemory(m_vkDevice, matStagingMem[i], 0, matStageCurrentSize[i], 0, &matStagingPtr[i]);

			// cameraData
			VkBufferCreateInfo camDatabufferInfo{};
			camDatabufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			camDatabufferInfo.size = sizeof(CameraData);
			camDatabufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			camDatabufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &camDatabufferInfo, nullptr, &camDataBuffer[i]));

			//VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(m_vkDevice, camDataBuffer[i], &memRequirements);

			VkMemoryAllocateInfo globalDataAllocInfo{};
			globalDataAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			globalDataAllocInfo.allocationSize = memRequirements.size;
			globalDataAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
																 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																 m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &globalDataAllocInfo, nullptr, &camDataMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, camDataBuffer[i], camDataMemory[i], 0));



			// ModelTransforms
			VkBufferCreateInfo modelTransBufferInfo{};
			modelTransBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			modelTransBufferInfo.size = sizeof(BaseMaterialInstance);
			modelTransBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			modelTransBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &modelTransBufferInfo, nullptr, &matBuf_modelTransforms[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, matBuf_modelTransforms[i], &memRequirements);

			VkMemoryAllocateInfo modelsAllocInfo{};
			modelsAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			modelsAllocInfo.allocationSize = memRequirements.size;
			modelsAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
															 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
															 m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &modelsAllocInfo, nullptr, &matDevMem_modelTransforms[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, matBuf_modelTransforms[i], matDevMem_modelTransforms[i], 0));


			// Instance Stage Indices
			//instanceIndexStage.resize(VULKAN_INSTANCE_INDEX_STAGE_SIZE);
			VkBufferCreateInfo indicesBufferInfo{};
			indicesBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			indicesBufferInfo.size = sizeof(uint32_t) * VULKAN_INSTANCE_INDEX_STAGE_SIZE;//instanceIndexStage.size();
			indicesBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			indicesBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &indicesBufferInfo, nullptr, &instanceIndexBuffer[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, instanceIndexBuffer[i], &memRequirements);

			VkMemoryAllocateInfo indexAllocInfo{};
			indexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			indexAllocInfo.allocationSize = memRequirements.size;
			indexAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
															VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
															m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &indexAllocInfo, nullptr, &instanceIndexMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, instanceIndexBuffer[i], instanceIndexMemory[i], 0));






			// Lights buffers
			uint32_t maxClusters = VULKAN_LIGHT_CLUSTERS_X * VULKAN_LIGHT_CLUSTERS_Y * VULKAN_LIGHT_CLUSTERS_Z;

			VkBufferCreateInfo lightsCountBufferInfo{};
			lightsCountBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			lightsCountBufferInfo.size = sizeof(Index128) * maxClusters;
			lightsCountBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			lightsCountBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &lightsCountBufferInfo, nullptr, &lightsClusterCountBuffer[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, lightsClusterCountBuffer[i], &memRequirements);

			VkMemoryAllocateInfo lCountsAllocInfo{};
			lCountsAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			lCountsAllocInfo.allocationSize = memRequirements.size;
			lCountsAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
															  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															m_phyDevice);
			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &lCountsAllocInfo, nullptr, &lightsClusterCountMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, lightsClusterCountBuffer[i], lightsClusterCountMemory[i], 0));



			VkBufferCreateInfo lightsIndicesBufferInfo{};
			lightsIndicesBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			lightsIndicesBufferInfo.size = sizeof(uint32_t) * maxClusters;
			lightsIndicesBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			lightsIndicesBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &lightsCountBufferInfo, nullptr, &lightsClusterIndicesBuffer[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, lightsClusterIndicesBuffer[i], &memRequirements);

			VkMemoryAllocateInfo lIndexAllocInfo{};
			lIndexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			lIndexAllocInfo.allocationSize = memRequirements.size;
			lIndexAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
															  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															  m_phyDevice);
			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &lIndexAllocInfo, nullptr, &lightsClusterIndicesMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, lightsClusterIndicesBuffer[i], lightsClusterIndicesMemory[i], 0));



			// Shapes
			VkBufferCreateInfo shapeVertbufferInfo{};
			shapeVertbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			shapeVertbufferInfo.size = sizeof(glm::vec3) * 128;
			shapeVertbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			shapeVertbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &shapeVertbufferInfo, nullptr, &shapeVertexBuffer[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, shapeVertexBuffer[i], &memRequirements);

			VkMemoryAllocateInfo shapeVertAllocInfo{};
			shapeVertAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			shapeVertAllocInfo.allocationSize = memRequirements.size;
			shapeVertAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
																 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																 m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &shapeVertAllocInfo, nullptr, &shapeVertexMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, shapeVertexBuffer[i], shapeVertexMemory[i], 0));

			VkBufferCreateInfo shapeIndexbufferInfo{};
			shapeIndexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			shapeIndexbufferInfo.size = sizeof(uint32_t) * 512;
			shapeIndexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			shapeIndexbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &shapeIndexbufferInfo, nullptr, &shapeIndexBuffer[i]));

			vkGetBufferMemoryRequirements(m_vkDevice, shapeIndexBuffer[i], &memRequirements);

			VkMemoryAllocateInfo shapeIndexAllocInfo{};
			shapeIndexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			shapeIndexAllocInfo.allocationSize = memRequirements.size;
			shapeIndexAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
																VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																m_phyDevice);

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &shapeIndexAllocInfo, nullptr, &shapeIndexMemory[i]));
			Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, shapeIndexBuffer[i], shapeIndexMemory[i], 0));
		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	DescriptorCaps queryDescriptorCaps(VkPhysicalDevice phys) {
		DescriptorCaps caps{};

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(phys, &props);
		caps.maxSetUBOs = props.limits.maxDescriptorSetUniformBuffers;
		caps.maxSetSSBOs = props.limits.maxDescriptorSetStorageBuffers;
		caps.maxSetSampledImages = props.limits.maxDescriptorSetSampledImages;
		caps.maxSetSamplers = props.limits.maxDescriptorSetSamplers;

		// If you use descriptor indexing / UPDATE_AFTER_BIND, query props2:
		VkPhysicalDeviceDescriptorIndexingProperties indexingProps{
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES
		};
		VkPhysicalDeviceProperties2 props2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		props2.pNext = &indexingProps;
		vkGetPhysicalDeviceProperties2(phys, &props2);

		// If the extension/feature is present these will be non-zero and usable.
		caps.maxUAB_SampledImages = indexingProps.maxDescriptorSetUpdateAfterBindSampledImages;
		caps.maxUAB_UniformBuffers = indexingProps.maxDescriptorSetUpdateAfterBindUniformBuffers;
		caps.maxUAB_StorageBuffers = indexingProps.maxDescriptorSetUpdateAfterBindStorageBuffers;
		caps.maxUAB_SamplerBuffers = indexingProps.maxDescriptorSetUpdateAfterBindSamplers;

		// You should also check features to know if UPDATE_AFTER_BIND is actually supported:
		VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
		};
		VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		features2.pNext = &indexingFeatures;
		vkGetPhysicalDeviceFeatures2(phys, &features2);

		caps.hasIndexing = indexingFeatures.descriptorBindingPartiallyBound ||
			indexingFeatures.runtimeDescriptorArray ||
			indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind ||
			indexingFeatures.descriptorBindingSampledImageUpdateAfterBind ||
			indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind;

		return caps;
	}

	inline VkResult createDescriptorPool() {
		VkResult vkResult;
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Descriptor pools... ");

		m_currentCaps = queryDescriptorCaps(m_phyDevice);

		//std::vector<VkDescriptorPoolSize> poolSizes = {
		//	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     100 },
		//	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,     200 },
		//	{ VK_DESCRIPTOR_TYPE_SAMPLER,            128 },
		//	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,      VULKAN_MAX_TEXTURE_SSBO }
		//};

		//VkDescriptorPoolCreateInfo poolInfo{};
		//poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		//poolInfo.maxSets = 256;
		//poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		//poolInfo.pPoolSizes = poolSizes.data();

		//Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &currentDescPool));
		//descPools.push_back(currentDescPool);

		std::vector<VkDescriptorPoolSize> baseSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     VULKAN_DESCPOOL_BASE_UNIFORMS},
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,     VULKAN_DESCPOOL_BASE_STORAGE },
			{ VK_DESCRIPTOR_TYPE_SAMPLER,            VULKAN_DESCPOOL_BASE_SAMPLER },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,      VULKAN_DESCPOOL_BASE_IMAGE }
		};
		//std::vector<VkDescriptorPoolSize> instancesSizes = {
		//	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     VULKAN_DESCPOOL_INSTANCE_UNIFORMS },
		//	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,     VULKAN_DESCPOOL_INSTANCE_STORAGE }
		//};
		//std::vector<VkDescriptorPoolSize> texturesSizes = {
		//	{ VK_DESCRIPTOR_TYPE_SAMPLER,            VULKAN_DESCPOOL_TEXTURES_SAMPLER },
		//	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,      VULKAN_DESCPOOL_TEXTURES_IMAGE }
		//};
		std::vector<VkDescriptorPoolSize> dynamicSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     VULKAN_DESCPOOL_DYNAMIC_UNIFORMS},
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,     VULKAN_DESCPOOL_DYNAMIC_STORAGE },
			{ VK_DESCRIPTOR_TYPE_SAMPLER,            VULKAN_DESCPOOL_DYNAMIC_SAMPLER },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,      VULKAN_DESCPOOL_DYNAMIC_IMAGE }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.maxSets = 1024;



		poolInfo.poolSizeCount = static_cast<uint32_t>(baseSizes.size());
		poolInfo.pPoolSizes = baseSizes.data();
		VkDescriptorPool basePool1;
		Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &basePool1));
		descriptorPoolsDefault[0] = basePool1;

		descriptorPoolsDefault[1] = descriptorPoolsDefault[0]; // hacky for now

		//VkDescriptorPool basePool2;
		//Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &basePool2));
		//descriptorPoolsDefault[1] = basePool2;

		//poolInfo.poolSizeCount = static_cast<uint32_t>(instancesSizes.size());
		//poolInfo.pPoolSizes = instancesSizes.data();
		//VkDescriptorPool instancePool;
		//Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &instancePool));
		//descriptorPoolsDefault[DESCPOOL_INSTANCE_INDEX] = instancePool;

		//poolInfo.poolSizeCount = static_cast<uint32_t>(texturesSizes.size());
		//poolInfo.pPoolSizes = texturesSizes.data();
		//VkDescriptorPool texturePool;
		//Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &texturePool));
		//descriptorPoolsDefault[DESCPOOL_TEXTURES_INDEX] = texturePool;

		poolInfo.poolSizeCount = static_cast<uint32_t>(dynamicSizes.size());
		poolInfo.pPoolSizes = dynamicSizes.data();
		VkDescriptorPool dynamicPool;
		Vk_CHECK(vkResult, vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &dynamicPool));
		descriptorPoolsDynamic.push_back(dynamicPool);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	INLINE VkResult createInstanceGlfw() {
		VkResult vkResult;
		LOGLINE_IND(LogType::Info, LogMod::Vulkan, "Creating VkInstance with GLFW... ", 1);

		glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_VULKAN);
		glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_PREFER_LIBDECOR);
		glfwInitHint(GLFW_X11_XCB_VULKAN_SURFACE, GLFW_TRUE);
#ifndef BUILD_WIN
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#endif

		if (!glfwInit()) {
			return (VkResult)ErrorCode::GLFW_UNKNOWN_INIT_ERROR;
		}

		PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)
			glfwGetInstanceProcAddress(NULL, "vkCreateInstance");

		PFN_vkCreateDevice pfnCreateDevice = (PFN_vkCreateDevice)
			glfwGetInstanceProcAddress(m_vkInstance, "vkCreateDevice");

		GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);

		return VK_SUCCESS;
	}

	inline VkResult createInstance() {
		VkResult vkResult;
		LOGLINE_IND(LogType::Info, LogMod::Vulkan, "Creating VkInstance... ", 1);
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_3;
		appInfo.pApplicationName = windowSurface->appName.c_str();

		// Create instance
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
#ifdef VULKAN_VALIDATION
		const char* extensions[] = {
			"VK_KHR_surface",
			"VK_KHR_win32_surface",
			"VK_EXT_debug_utils"
		};
		instanceCreateInfo.enabledExtensionCount = 3;

		if (checkValidationLayerSupport()) {
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			instanceCreateInfo.enabledLayerCount = 0;
		}
		LOGLINE(LogType::Info, LogMod::Vulkan, "Validation is ON. ");
#else
		const char* extensions[] = {
			"VK_KHR_surface",
			"VK_KHR_win32_surface",
		};
		instanceCreateInfo.enabledExtensionCount = 2;
		instanceCreateInfo.enabledLayerCount = 0;
		LOGLINE(LogType::Info, LogMod::Vulkan, "Validation is OFF. ");
#endif
		instanceCreateInfo.ppEnabledExtensionNames = extensions;
		Vk_CHECK(vkResult, vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance));

		LOGLINE_IND(LogType::Success, LogMod::Vulkan, "VkInstance created.", -1);
		return VK_SUCCESS;
	}

	inline VkResult createSurface() {
		VkResult vkResult;

		// Create surface
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Vk_Win32 Surface... ");
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = windowSurface->windowInstance;
		surfaceCreateInfo.hwnd = windowSurface->windowHandle;

		Vk_CHECK(vkResult, vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_vkSurface));
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult pickPhysDevice(bool prioIGpu) {
		VkResult vkResult{};
		// Enumerate physical devices
		LOGLINE(LogType::Info, LogMod::Vulkan, "Choosing GPU... ");
		uint32_t deviceCount = 0;
		Vk_CHECK(vkResult, vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr));
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		Vk_CHECK(vkResult, vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data()));

		std::string deviceName{};
		m_phyDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties deviceProperties;
		std::vector<uint16_t> scores;
		uint16_t bestScore = 0;
		scores.resize(physicalDevices.size());
		for (size_t d = 0; d < physicalDevices.size(); ++d) {
			auto& device = physicalDevices[d];
			scores[d] = 0;
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
			for (uint32_t i = 0; i < queueFamilyCount; i++) {
				bool supportsGraphics = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
				VkBool32 supportsPresent = VK_FALSE;
				Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &supportsPresent));

				if (supportsGraphics && supportsPresent) {
					scores[d] += 1000;
					
					VkPhysicalDeviceFeatures deviceFeatures;
					vkGetPhysicalDeviceProperties(device, &deviceProperties);
					vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

					if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && !prioIGpu) {
							scores[d] += 1000;
					} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && prioIGpu) {
						scores[d] += 1000;
					}

					if (scores[d] > bestScore) {
						bestScore = scores[d];
						m_phyDevice = device;
						m_graphicsQueueFamilyIndex = i;
						deviceName = deviceProperties.deviceName;
						//deviceFeatures = deviceFeatures;
					}
				}
			}
		}

		if (m_phyDevice == VK_NULL_HANDLE)
			return VK_ERROR_INITIALIZATION_FAILED;

		LOG(LogType::Remark, "Using " + deviceName);
		return VK_SUCCESS;
	}

	INLINE VkResult createLogicalDevice() {
		VkResult vkResult{};

		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating logical device... ");

		// --- Queue ---
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCI{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCI.queueFamilyIndex = m_graphicsQueueFamilyIndex;
		queueCI.queueCount = 1;
		queueCI.pQueuePriorities = &queuePriority;

		// --- Query support (1.3 -> 1.2 chain). Do NOT include legacy feature structs. ---
		VkPhysicalDeviceVulkan13Features supp13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		VkPhysicalDeviceVulkan12Features supp12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

		supp13.pNext = &supp12;

		VkPhysicalDeviceFeatures2 supp{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		supp.pNext = &supp13;

		vkGetPhysicalDeviceFeatures2(m_phyDevice, &supp);

		// --- Require what you actually use ---
		// Descriptor indexing subset (core in 1.2)
		if (!supp12.descriptorIndexing ||
			!supp12.runtimeDescriptorArray ||
			!supp12.shaderSampledImageArrayNonUniformIndexing ||
			!supp12.descriptorBindingPartiallyBound ||
			!supp12.descriptorBindingVariableDescriptorCount) {
			LOG(LogType::Error, "Required descriptor indexing features not supported.");
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		// Scalar block layout (needed for -fvk-use-dx-layout)
		if (!supp12.scalarBlockLayout) {
			LOG(LogType::Error, "scalarBlockLayout not supported.");
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		// for dynamic states
		VkPhysicalDeviceExtendedDynamicState2FeaturesEXT ext2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT };
		ext2.extendedDynamicState2 = VK_TRUE;
		ext2.extendedDynamicState2LogicOp = VK_TRUE;
		ext2.extendedDynamicState2PatchControlPoints = VK_TRUE;

		// --- Build ENABLE chain (separate structs from the query) ---
		VkPhysicalDeviceVulkan12Features en12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		en12.descriptorIndexing = VK_TRUE;
		en12.runtimeDescriptorArray = VK_TRUE;
		en12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		en12.descriptorBindingPartiallyBound = VK_TRUE;
		en12.descriptorBindingVariableDescriptorCount = VK_TRUE;
		en12.scalarBlockLayout = VK_TRUE;
		en12.pNext = &ext2;

		VkPhysicalDeviceVulkan13Features en13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		en13.shaderDemoteToHelperInvocation = supp13.shaderDemoteToHelperInvocation; // enable if supported
		en13.synchronization2 = VK_TRUE;
		en13.pNext = &en12;

		VkPhysicalDeviceFeatures2 enable{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		enable.pNext = &en13;
		enable.features.samplerAnisotropy = VK_TRUE;
		enable.features.vertexPipelineStoresAndAtomics = VK_TRUE;
		enable.features.fragmentStoresAndAtomics = VK_TRUE;

		// --- Extensions: swapchain is enough on 1.2+; DO NOT enable VK_EXT_descriptor_indexing here. ---
		std::vector<const char*> exts;
		exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo devCI{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		devCI.queueCreateInfoCount = 1;
		devCI.pQueueCreateInfos = &queueCI;
		devCI.enabledExtensionCount = static_cast<uint32_t>(exts.size());
		devCI.ppEnabledExtensionNames = exts.data();
		devCI.pNext = &enable;

		Vk_CHECK(vkResult, vkCreateDevice(m_phyDevice, &devCI, nullptr, &m_vkDevice));
		vkGetDeviceQueue(m_vkDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	//inline VkResult createLogicalDevice() {
	//	VkResult vkResult{};

	//	// Create logical device
	//	LOGLINE(LogType::Info, LogMod::Vulkan, "Creating logical device... ");
	//	float queuePriority = 1.0f;
	//	VkDeviceQueueCreateInfo queueCreateInfo = {};
	//	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	//	queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
	//	queueCreateInfo.queueCount = 1;
	//	queueCreateInfo.pQueuePriorities = &queuePriority;

	//	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
	//	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	//	indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	//	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	//	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	//	indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

	//	VkPhysicalDeviceVulkan13Features features13{};
	//	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	//	features13.shaderDemoteToHelperInvocation = VK_TRUE;

	//	// Chain features together
	//	features13.pNext = &indexingFeatures;

	//	// Wrap in PhysicalDeviceFeatures2 to query support
	//	VkPhysicalDeviceFeatures2 features2{};
	//	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	//	features2.pNext = &features13;

	//	// Query supported features from the physical device
	//	vkGetPhysicalDeviceFeatures2(m_phyDevice, &features2);

	//	const char* deviceExtensions[] = { 
	//		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	//		"VK_EXT_descriptor_indexing"
	//	};

	//	VkDeviceCreateInfo deviceCreateInfo = {};
	//	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	//	deviceCreateInfo.queueCreateInfoCount = 1;
	//	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	//	deviceCreateInfo.enabledExtensionCount = std::size(deviceExtensions);
	//	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	//	deviceCreateInfo.pNext = &features2;

	//	Vk_CHECK(vkResult, vkCreateDevice(m_phyDevice, &deviceCreateInfo, nullptr, &m_vkDevice));
	//	vkGetDeviceQueue(m_vkDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

	//	LOG(LogType::Success, "Done.");
	//	return VK_SUCCESS;
	//}


	inline VkResult createSwapchain() {
		VkResult vkResult;

		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Swapchain... ");

		// Query surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCaps;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_phyDevice, m_vkSurface, &surfaceCaps));

		// Pick format
		uint32_t formatCount = 0;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(m_phyDevice, m_vkSurface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(m_phyDevice, m_vkSurface, &formatCount, formats.data()));

		VkSurfaceFormatKHR surfaceFormat = formats[0];
		for (const auto& fmt : formats) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
				fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = fmt;
				break;
			}
		}

		surfaceFormat = surfaceFormat;
		swapchainFormat = surfaceFormat.format;
		swapchainExtent = { surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height };

		// Swapchain create info
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_vkSurface;
		swapchainCreateInfo.minImageCount = surfaceCaps.minImageCount + 1;
		swapchainCreateInfo.imageFormat = swapchainFormat;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			//| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.preTransform = surfaceCaps.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;

		Vk_CHECK(vkResult, vkCreateSwapchainKHR(m_vkDevice, &swapchainCreateInfo, nullptr, &swapchain));
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createImageViews() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating ImageViews... ");
		VkResult vkResult;
		// Get swapchain images
		uint32_t imageCount = 0;
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(m_vkDevice, swapchain, &imageCount, nullptr));
		swapchainImages.resize(imageCount);
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(m_vkDevice, swapchain, &imageCount, swapchainImages.data()));

		swapchainImageViews.resize(swapchainImages.size());
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapchainFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			Vk_CHECK(vkResult, vkCreateImageView(m_vkDevice, &createInfo, nullptr, &swapchainImageViews[i]));
		}

		depthStencilImages.resize(swapchainImages.size());
		depthStencilViews.resize(swapchainImages.size());
		depthStencilMemories.resize(swapchainImages.size());
		for (size_t i = 0; i < depthStencilImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			imageInfo.extent = { swapchainExtent.width, swapchainExtent.height, 1 };
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			Vk_CHECK(vkResult, vkCreateImage(m_vkDevice, &imageInfo, nullptr, &depthStencilImages[i]));

			// --- BIND MEMORY ---
			VkMemoryRequirements memReq{};
			vkGetImageMemoryRequirements(m_vkDevice, depthStencilImages[i], &memReq);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memReq.size;

			// Find suitable memory type
			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(m_phyDevice, &memProps);
			for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j) {
				if ((memReq.memoryTypeBits & (1 << j)) &&
					(memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
					allocInfo.memoryTypeIndex = j;
					break;
				}
			}

			Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &depthStencilMemories[i]));
			Vk_CHECK(vkResult, vkBindImageMemory(m_vkDevice, depthStencilImages[i], depthStencilMemories[i], 0));

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthStencilImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = imageInfo.format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; // depends on format
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			Vk_CHECK(vkResult, vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &depthStencilViews[i]));


		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createShaderModule(Shader& shader, VkShaderModule* outShaderModule) {
		VkResult vkResult{};
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shader.bytecode.size() * sizeof(uint32_t);
		createInfo.pCode = shader.bytecode.data();
		Vk_CHECK(vkResult, vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, outShaderModule));
		return VK_SUCCESS;
	}

	inline VkResult createRenderPass() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating RenderPass... ");
		VkResult vkResult{};

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> refs;

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back(colorAttachment);

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		refs.push_back(colorAttachmentRef);

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;//VK_FORMAT_D32_SFLOAT; VK_FORMAT_D24_UNORM_S8_UINT
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments.push_back(depthAttachment);

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(refs.size());
		subpass.pColorAttachments = refs.data();
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkRenderPass basicRenderPass;
		VkRenderPassCreateInfo basicRenderPassInfo{};
		basicRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		basicRenderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		basicRenderPassInfo.pAttachments = attachments.data();
		basicRenderPassInfo.subpassCount = 1;
		basicRenderPassInfo.pSubpasses = &subpass;
		
		Vk_CHECK(vkResult, vkCreateRenderPass(m_vkDevice, &basicRenderPassInfo, nullptr, &basicRenderPass));

		rendPasses.push_back(basicRenderPass);




		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	VkResult createPipelineFromMaterial(RenderManager* const renderMan, Material& material) {
		LOGLINE(LogType::Info, LogMod::Vulkan, std::string{ "Creating Pipeline for " + material.name() + "... " });
		VkResult vkResult{};

		bool pipelineMatch = false;

		// Make Material ready
		auto key = PipelineKey::create(material);
		auto keyIt = pipelineHashIndexMap.find(key);
		if (keyIt != pipelineHashIndexMap.end()) {
			pipelineMatch = true;
			LOG(LogType::Success, std::format(" Reusing Pipeline Index: {}.. ", keyIt->second));
		}

		// Layout & Descriptor sets
		std::vector<BindSetCombo> descriptorSetsList;
		std::vector<VkDescriptorSetLayout> layouts;
		Vk_CHECK(vkResult, _initDescLayoutAndSets(renderMan, material, vkResult, layouts, descriptorSetsList, !pipelineMatch));

		if (pipelineMatch) {
			material.init(keyIt->second, keyIt->second, descriptorSetsList);
			return VK_SUCCESS;
		}

		// Shader Stages
		auto vs = renderMan->getShader(material.vShaderName);
		auto ps = renderMan->getShader(material.pShaderName);
		if (!vs || !ps)
			return VK_ERROR_UNKNOWN;

		VkShaderModule vertModule{};
		Vk_CHECK(vkResult, createShaderModule(*vs, &vertModule));
		VkShaderModule fragModule{};
		Vk_CHECK(vkResult, createShaderModule(*ps, &fragModule));

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertModule;
		vertShaderStageInfo.pName = vs->entryPoint.c_str();

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragModule;
		fragShaderStageInfo.pName = ps->entryPoint.c_str();

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo ,fragShaderStageInfo };



		// VS input description
		std::vector<VkVertexInputAttributeDescription> attributeDescs;
		uint32_t vsIaStride = 0;
		uint32_t iaLoc = 0;
		for (const auto& attr : vs->input.attributes) {
			if (attr.location == UINT8_INVALID)
				continue;
			VkVertexInputAttributeDescription desc{};
			desc.location = iaLoc++;//attr.location != UINT8_INVALID ? attr.location : 0;
			desc.binding = 0;
			desc.format = GetVkFormat(attr.type);
			desc.offset = attr.offset;
			uint32_t end = attr.offset + SizeOfTypeBase(attr.type);
			vsIaStride = std::max(vsIaStride, end);
			attributeDescs.push_back(desc);
		}
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = vsIaStride;
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();


		// IA State
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;


		// Rasterization State
		VkPipelineRasterizationStateCreateInfo rasterizer;
		if (material.rasterMode != RasterMode::Custom) {
			rasterizer = GetRasterPreset(material.rasterMode);
		} else {
			rasterizer = *static_cast<VkPipelineRasterizationStateCreateInfo*>(material.rasterModeCustomPtr);
		}

		// MSAA State
		VkPipelineMultisampleStateCreateInfo multisampling = GetMultisamplingPreset(material.msaaMode);

		// Depth Stencil State
		VkPipelineDepthStencilStateCreateInfo stencil;
		if (material.depthMode != DepthMode::Custom) {
			stencil = GetDepthStencilPreset(material.depthMode);
		} else {
			stencil = *static_cast<VkPipelineDepthStencilStateCreateInfo*>(material.depthModeCustomPtr);
		}

		// Color Blend State
		std::vector< VkPipelineColorBlendAttachmentState> colorBlends;
		for (size_t i = 0; i < material.blendModes.size(); ++i) {
			VkPipelineColorBlendAttachmentState colorBlend;
			if (material.blendModes[i] != BlendMode::Custom) {
				colorBlend = GetBlendPreset(material.blendModes[i]);
			} else {
				colorBlend = *static_cast<VkPipelineColorBlendAttachmentState*>(material.blendModeCustomPtrs[i]);
			}
			colorBlends.push_back(colorBlend);
		}
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = colorBlends.size();
		colorBlending.pAttachments = colorBlends.data();

		
		// Push Constants
		std::vector<VkPushConstantRange> pushConstantRanges;
		for (const auto& param : material.params) {
			if (param.type != TypeBase::PushConstStruct)
				continue;

			VkPushConstantRange range{};
			range.offset = param.offset;
			range.size = param.size;
			range.stageFlags = MatParamStageToVkShaderStageFlagBits(param.stage, param.resourceType);
			pushConstantRanges.push_back(range);
		}

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		layoutInfo.pSetLayouts = layouts.data();
		layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		layoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkPipelineLayout pipelineLayout;
		Vk_CHECK(vkResult, vkCreatePipelineLayout(m_vkDevice, &layoutInfo, nullptr, &pipelineLayout));


		// Create the pipeline
		// Dynamic State
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
			VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
			VK_DYNAMIC_STATE_DEPTH_COMPARE_OP
		};

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &stencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = rendPasses[0];
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		VkPipeline graphicsPipeline;
		Vk_CHECK(vkResult, vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

		// Make Material ready
		auto pipelineIndex = pipelines.size();
		auto pipelineLayoutIndex = pipelineLayouts.size();
		auto remakeKey = PipelineKey::create(material);

		material.init(pipelineIndex, pipelineLayoutIndex, descriptorSetsList);
		pipelines.push_back(graphicsPipeline);
		pipelineLayouts.push_back(pipelineLayout);
		pipelineHashIndexMap.insert({ remakeKey, static_cast<uint32_t>(pipelineIndex) });
		LOG(LogType::Success, "Done.");
		

		
		return VK_SUCCESS;
	}

	inline VkResult createFramebuffers() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Framebuffers... ");
		VkResult vkResult{};

		swapChainFramebuffers.clear();
		swapChainFramebuffers.resize(swapchainImageViews.size());


		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapchainImageViews[i],
				depthStencilViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = rendPasses[renderPassIndex];
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapchainExtent.width;
			framebufferInfo.height = swapchainExtent.height;
			framebufferInfo.layers = 1;

			Vk_CHECK(vkResult, vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createCommandPool() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command pool... ");
		VkResult vkResult{};

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_phyDevice, m_vkSurface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		Vk_CHECK(vkResult, vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &commandPool));

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createCommandBuffer() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command buffers... ");
		VkResult vkResult{};

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			Vk_CHECK(vkResult, vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &frameSync[i].cmdBuffer));
		}
		
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createSyncObjects() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Sync objects... ");
		VkResult vkResult{};

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < VULKAN_FRAMES_IN_FLIGHT; ++i) {
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &frameSync[i].imageAvailable));
			Vk_CHECK(vkResult, vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &frameSync[i].inFlight));
		}
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			imageRenderFinished.push_back(VkSemaphore{});
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &imageRenderFinished.back()));
		}
		
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createSamplerPresets() {
		VkResult vkResult;

		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Sampler presets... ");

		VkSampler sampler;

		// point
		VkSamplerCreateInfo point_repeat = SamplerStatesPresets::point;
		VkSamplerCreateInfo point_mirrorRepeat = point_repeat;
		point_mirrorRepeat.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		point_mirrorRepeat.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		point_mirrorRepeat.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		VkSamplerCreateInfo point_clampEdge = point_repeat;
		point_clampEdge.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		point_clampEdge.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		point_clampEdge.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerCreateInfo point_clampBorder = point_repeat;
		point_clampBorder.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		point_clampBorder.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		point_clampBorder.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &point_repeat, nullptr, &sampler));
		baseSamplers.insert( { {SamplerMode::Point, SamplerAddressMode::Repeat}, sampler } );
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &point_mirrorRepeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Point, SamplerAddressMode::MirroredRepeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &point_clampEdge, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Point, SamplerAddressMode::ClampEdge}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &point_clampBorder, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Point, SamplerAddressMode::ClampBorder}, sampler });
		
		// Linear
		VkSamplerCreateInfo linear_repeat = SamplerStatesPresets::linear;
		VkSamplerCreateInfo linear_mirrorRepeat = linear_repeat;
		linear_mirrorRepeat.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		linear_mirrorRepeat.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		linear_mirrorRepeat.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		VkSamplerCreateInfo linear_clampEdge = linear_repeat;
		linear_clampEdge.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		linear_clampEdge.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		linear_clampEdge.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerCreateInfo linear_clampBorder = linear_repeat;
		linear_clampBorder.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		linear_clampBorder.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		linear_clampBorder.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &linear_repeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Linear, SamplerAddressMode::Repeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &linear_mirrorRepeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Linear, SamplerAddressMode::MirroredRepeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &linear_clampEdge, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Linear, SamplerAddressMode::ClampEdge}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &linear_clampBorder, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Linear, SamplerAddressMode::ClampBorder}, sampler });


		// Anisotropic
		VkSamplerCreateInfo aniso_repeat = SamplerStatesPresets::aniso;
		aniso_repeat.maxAnisotropy = 4.0f;
		VkSamplerCreateInfo aniso_mirrorRepeat = aniso_repeat;
		aniso_mirrorRepeat.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		aniso_mirrorRepeat.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		aniso_mirrorRepeat.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		VkSamplerCreateInfo aniso_clampEdge = aniso_repeat;
		aniso_clampEdge.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		aniso_clampEdge.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		aniso_clampEdge.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerCreateInfo aniso_clampBorder = aniso_repeat;
		aniso_clampBorder.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		aniso_clampBorder.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		aniso_clampBorder.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_repeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso4X, SamplerAddressMode::Repeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_mirrorRepeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso4X, SamplerAddressMode::MirroredRepeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampEdge, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso4X, SamplerAddressMode::ClampEdge}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampBorder, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso4X, SamplerAddressMode::ClampBorder}, sampler });

		aniso_repeat.maxAnisotropy = 8.0f;
		aniso_mirrorRepeat.maxAnisotropy = 8.0f;
		aniso_clampEdge.maxAnisotropy = 8.0f;
		aniso_clampBorder.maxAnisotropy = 8.0f;
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_repeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso8X, SamplerAddressMode::Repeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_mirrorRepeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso8X, SamplerAddressMode::MirroredRepeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampEdge, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso8X, SamplerAddressMode::ClampEdge}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampBorder, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso8X, SamplerAddressMode::ClampBorder}, sampler });

		aniso_repeat.maxAnisotropy = 16.0f;
		aniso_mirrorRepeat.maxAnisotropy = 16.0f;
		aniso_clampEdge.maxAnisotropy = 16.0f;
		aniso_clampBorder.maxAnisotropy = 16.0f;
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_repeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso16X, SamplerAddressMode::Repeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_mirrorRepeat, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso16X, SamplerAddressMode::MirroredRepeat}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampEdge, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso16X, SamplerAddressMode::ClampEdge}, sampler });
		Vk_CHECK(vkResult, vkCreateSampler(m_vkDevice, &aniso_clampBorder, nullptr, &sampler));
		baseSamplers.insert({ {SamplerMode::Aniso16X, SamplerAddressMode::ClampBorder}, sampler });

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}



	inline VkResult recreateSwapchain() {
		VkResult vkResult{};

		LOGLINE_IND(LogType::Info, LogMod::Vulkan, "Recreating Swapchain... ", 1);

		vkDeviceWaitIdle(m_vkDevice);

		cleanupSyncObjects();
		cleanUpSwapchainOnly();

		Vk_CHECK(vkResult, createSwapchain());
		Vk_CHECK(vkResult, createImageViews());
		Vk_CHECK(vkResult, createFramebuffers());

		createSyncObjects();

		pendingResize = false;

		LOGLINE_IND(LogType::Success, LogMod::Vulkan, "Swapchain recreated.", -1);
		return VK_SUCCESS;
	}

	inline void cleanUpSwapchainOnly() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(m_vkDevice, swapChainFramebuffers[i], nullptr);
		}
		swapChainFramebuffers.clear();

		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			vkDestroyImageView(m_vkDevice, swapchainImageViews[i], nullptr);
		}
		swapchainImageViews.clear();

		vkDestroySwapchainKHR(m_vkDevice, swapchain, nullptr);
	}

	inline void cleanupSyncObjects() {
		for (auto& sync : frameSync) {
			vkDestroySemaphore(m_vkDevice, sync.imageAvailable, nullptr);
			vkDestroyFence(m_vkDevice, sync.inFlight, nullptr);
		}

		//for (auto& s : imageRenderDone)
		//	vkDestroySemaphore(m_vkDevice, s, nullptr);
		//imageRenderDone.clear();
	}
	
	inline VkResult resetPipeline(size_t pipelineIndex, Shader& vs, Shader& ps) {
		//VkResult vkResult{};

		//LOGLINE_IND(LogType::Info, LogMod::Vulkan, "Hot Reloading shaders... ", 1);

		//Vk_CHECK(vkResult, vkDeviceWaitIdle(vkDevice));
		//vkDestroyPipelineLayout(vkDevice, pipelineLayouts[pipelineIndex], nullptr);
		//vkDestroyPipeline(vkDevice, pipelines[pipelineIndex], nullptr);

		//Vk_CHECK(vkResult, createGraphicsPipeline(vs, ps, pipelineIndex));

		//LOGLINE_IND(LogType::Success, LogMod::Vulkan, "Hot Reload completed.", -1);
		return VK_SUCCESS;

	}

	inline void cleanUp() {

		LOGLINE(LogType::Info, LogMod::Vulkan, "Cleaning up... ");

		vkDeviceWaitIdle(m_vkDevice);

		//for (auto& pipeline : pipelines)
		//	vkDestroyPipeline(m_vkDevice, pipeline, nullptr);
		//pipelines.clear();

		//for (auto& layout : pipelineLayouts)
		//	vkDestroyPipelineLayout(m_vkDevice, layout, nullptr);
		//pipelineLayouts.clear();

		for (auto& pass : rendPasses)
			vkDestroyRenderPass(m_vkDevice, pass, nullptr);
		rendPasses.clear();

		cleanUpSwapchainOnly();

		cleanupSyncObjects();

		vkDestroyCommandPool(m_vkDevice, commandPool, nullptr);

		vkDestroyDevice(m_vkDevice, nullptr);

		vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

		vkDestroyInstance(m_vkInstance, nullptr);

		//vkDestroyDescriptorSetLayout(m_vkDevice, bindlessTextureSetLayout, nullptr);

		isClean = true;

		LOG(LogType::Success, "Done.");
	}

	inline VkResult recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		//LOGLINE(LogType::Info, LogMod::Vulkan, "Recording command buffer... ");
		VkResult vkResult{};

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		Vk_CHECK(vkResult, vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}



	inline void bindMaterialParameters(
		VkCommandBuffer cmd,
		Material* material,
		std::vector<VkDescriptorSet>& descriptorSetsBySetIndex, // Assumed to be pre-filled
		const void* pushConstData = nullptr,
		uint32_t pushConstSize = 0) {
		assert(material && pipelineLayouts[material->pipelineLayoutId]);

		// Bind descriptor sets (grouped by set index)
		if (!descriptorSetsBySetIndex.empty()) {
			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[material->pipelineLayoutId],
				0, // First set
				static_cast<uint32_t>(descriptorSetsBySetIndex.size()),
				descriptorSetsBySetIndex.data(),
				0, nullptr // Dynamic offsets (if used)
			);
		}

		// Push constants (if any)
		for (const MatParam& param : material->params) {
			if (param.type == TypeBase::PushConst || param.type == TypeBase::PushConstStruct) {
				if (pushConstData && pushConstSize > 0) {
					vkCmdPushConstants(
						cmd,
						pipelineLayouts[material->pipelineLayoutId],
						MatParamStageToVkShaderStageFlagBits(param.stage, param.resourceType),
						param.offset,
						static_cast<uint32_t>(param.size),
						static_cast<const uint8_t*>(pushConstData) + param.offset
					);
				}
			}
		}
	}


	void draw(const DrawContext& rendCtx);

	inline void notifyViewResized(void* ctx, uint16_t width, uint16_t height) {
		pendingResize = true;
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}


	VkResult transferTextureData(void* pixelData, const VkImage& image, uint8_t mipLevels,
								 uint16_t width, uint16_t height, uint8_t bytesPerPixel,
								 VkCommandPool cmdPool, VkQueue queue) {

		VkResult vkResult{};

		VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.commandBufferCount = 1;

		VkCommandBuffer loadingCmdBuffer;
		Vk_CHECK(vkResult, vkAllocateCommandBuffers(m_vkDevice, &cmdBufferAllocInfo, &loadingCmdBuffer));

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		uint32_t imageSize = width * height * bytesPerPixel;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Vk_CHECK(vkResult, vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &stagingBuffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_vkDevice, stagingBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
												   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_phyDevice);

		Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &stagingMemory));
		Vk_CHECK(vkResult, vkBindBufferMemory(m_vkDevice, stagingBuffer, stagingMemory, 0));
		void* data;
		Vk_CHECK(vkResult, vkMapMemory(m_vkDevice, stagingMemory, 0, imageSize, 0, &data));
		memcpy(data, pixelData, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_vkDevice, stagingMemory);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;



		vkBeginCommandBuffer(loadingCmdBuffer, &beginInfo);

		vkCmdPipelineBarrier(loadingCmdBuffer,
							 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(loadingCmdBuffer, stagingBuffer, image,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1; i < mipLevels; ++i) {
			// Transition level i-1 to TRANSFER_SRC_OPTIMAL
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseMipLevel = i - 1;

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(loadingCmdBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
								 0, 0, nullptr, 0, nullptr, 1, &barrier);

			// Set up the blit
			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = {
				std::max(1, mipWidth / 2),
				std::max(1, mipHeight / 2),
				1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(loadingCmdBuffer,
						   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1, &blit, VK_FILTER_LINEAR);

			// After blit, transition previous mip level to SHADER_READ_ONLY_OPTIMAL
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(loadingCmdBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								 0, 0, nullptr, 0, nullptr, 1, &barrier);

			mipWidth = std::max(1, mipWidth / 2);
			mipHeight = std::max(1, mipHeight / 2);
		}

		VkImageMemoryBarrier lastBarrier = barrier;
		lastBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
		lastBarrier.subresourceRange.levelCount = 1;
		lastBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		lastBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lastBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		lastBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(loadingCmdBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							 0, 0, nullptr, 0, nullptr, 1, &lastBarrier);

		vkEndCommandBuffer(loadingCmdBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &loadingCmdBuffer;

		VkFence fence;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		Vk_CHECK(vkResult, vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &fence));

		Vk_CHECK(vkResult, vkQueueSubmit(queue, 1, &submitInfo, fence));

		Vk_CHECK(vkResult, vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(m_vkDevice, fence, nullptr);
		vkFreeCommandBuffers(m_vkDevice, cmdPool, 1, &loadingCmdBuffer);

		vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_vkDevice, stagingMemory, nullptr);

		return vkResult;
	}


	VkResult loadTexture(const Texture2D& tex, uint32_t* outTexIndex) {
		VkResult vkResult{};

		uint32_t mipLevels = tex.generateMips
			? static_cast<uint32_t>(
				std::floor(std::log2(std::max(tex.width, tex.height)))
				) + 1
			: 1;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = tex.width;
		imageInfo.extent.height = tex.height;
		imageInfo.extent.depth = tex.depth;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = tex.arrayLayers;
		imageInfo.format = tex.format;
		imageInfo.tiling = tex.tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = tex.usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		VkImage image;
		Vk_CHECK(vkResult, vkCreateImage(m_vkDevice, &imageInfo, nullptr, &image));

		VkMemoryRequirements memReq;
		vkGetImageMemoryRequirements(m_vkDevice, image, &memReq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_phyDevice);

		VkDeviceMemory memory;
		Vk_CHECK(vkResult, vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &memory));
		Vk_CHECK(vkResult, vkBindImageMemory(m_vkDevice, image, memory, 0));

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = imageInfo.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		Vk_CHECK(vkResult, vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView));


		VkTextureResource texture{};
		texture.image = image;
		texture.imageView = imageView;
		texture.imageInfo.imageView = imageView;
		texture.imageInfo.imageLayout = tex.imageLayout;
		texture.imageInfo.sampler = baseSamplers[{SamplerMode::Aniso8X, SamplerAddressMode::Repeat}];

		uint32_t texIndex = static_cast<uint32_t>(texResources.size());

		if (outTexIndex) {
			*outTexIndex = texIndex;
		}
		texResources.push_back(texture);

		// update descriptor sets
		for (size_t t = 0; t < VULKAN_FRAMES_IN_FLIGHT; ++t) {
			VkDescriptorSet descSet = bindingToDescriptorSet[{MAT_TEXTURES_BIND, MAT_TEXTURES_SET, 0}][t];
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = descSet;
			write.dstBinding = MAT_TEXTURES_BIND; // binding = 3 in your shader
			write.dstArrayElement = texIndex;  // index in textures[]
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // or COMBINED_IMAGE_SAMPLER
			write.descriptorCount = 1;
			write.pImageInfo = &texture.imageInfo;

			vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
		}


		return transferTextureData(tex.texelData(), texture.image, mipLevels, tex.width, tex.height, 4, commandPool, m_graphicsQueue);
	}


	void preDraw(RenderManager* const renderMan);
	void postDraw();
};
#pragma warning(pop)
#endif