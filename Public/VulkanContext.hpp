#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#pragma warning(push)
#pragma warning(disable : 28251)

#ifdef BUILD_WIN
	#ifndef _INC_WINAPIFAMILY
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include <windows.h>
	#endif
#endif


#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <array>
#include <unordered_map>

#include "WindowSurface.h"
#include "Log.hpp"

#include "GraphicContext.h"
#include "Material.hpp"
#include "IShaderProvider.h"

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
		case MultiSamplingMode::MSAA_None:
			return VkMultiSamplingStates::MSAA_1x;
		case MultiSamplingMode::MSAA_4x:
			return VkMultiSamplingStates::MSAA_4x;
	}
}

class VulkanContext : public GraphicContext
{
public:
	struct RenderContext
	{
		float clearColor[4] = {0.0f, 0.12f, 0.55f, 1.0f};
		uint16_t renderPassIndex = 0;
		uint16_t pipelineIndex = 0;
		float vPortPos[2] = {0,0};
		float vPortSize[2] = {-1, -1};
		float vPortMinDepth = 0.0f;
		float vPortMaxDepth = 1.0f;
		int sciOffset[2] = {0,0};
		int sciSize[2] = {-1, -1};
		
		uint64_t frameCount;
	};
	struct FrameSync
	{
		VkSemaphore imageAvailable;
		VkFence inFlight;
		VkCommandBuffer cmdBuffer;
	};

private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

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

public:
	
	VkInstance m_vkInstance = nullptr;
	VkSurfaceKHR m_vkSurface = nullptr;
	VkPhysicalDevice m_phyDevice = nullptr;
	VkDevice m_vkDevice = nullptr;
	VkQueue m_graphicsQueue = nullptr;
	uint32_t m_graphicsQueueFamilyIndex = static_cast<uint32_t>(-1);

	//std::unordered_map<std::string, PipelineDesc> m_namedPipelines;

	BlendMode currentBlendMode = BlendMode::Opaque;

	uint32_t renderPassIndex = 0;
	bool isClean = true;
	uint8_t currentFrame = 0;
	bool pendingResize = false;

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

	
	VkDescriptorSetLayout bindlessTextureSetLayout;
	VkPipelineLayout spritePipelineLayout;
	VkRenderPass spriteRenderPass;
	VkPipeline spritePipeline;

	VkDescriptorPool descriptorPool;
	
	std::unordered_map<size_t, size_t> matIndexPipelineIndexMap;
	std::vector<VkPipelineLayout> pipelineLayouts;
	std::vector<VkRenderPass> rendPasses;
	std::vector<VkPipeline> pipelines;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool = nullptr;
	std::array<FrameSync, VULKAN_MAX_FRAMES_IN_FLIGHT> frameSync;
	std::vector<VkSemaphore> imageRenderDone;

	virtual ~VulkanContext() {
		if (!isClean)
			cleanUp();
	}
	VulkanContext() = delete;
	inline VulkanContext(WindowSurface* const wndSurface) :
		GraphicContext(wndSurface),
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
		renderPassIndex{}
	{
		pipelineLayouts.reserve(2048);
		rendPasses.reserve(2048);
		pipelines.reserve(2048);
	}

	inline VkResult initVulkan(const VkPresentModeKHR mode, bool prioIGpu = false) {
		VkResult vk{};

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
			frameSync[i] = FrameSync{};
		}
		presentMode = mode;
		renderPassIndex = 0;
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

		isClean = false;

		return VK_SUCCESS;
	}

	//inline VkResult createOrReloadPipeline(PsoDesc& pso) {
	//	VkResult vk{};
	//	Vk_CHECK(vk, createGraphicsPipeline(pso));
	//	return VK_SUCCESS;
	//}

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
						//deviceFeatures = deviceFeatures;
					}
				}
			}
		}

		if (m_phyDevice == VK_NULL_HANDLE)
			return VK_ERROR_INITIALIZATION_FAILED;

		LOG(LogType::Remark, "Using " + std::string{ deviceProperties.deviceName });
		return VK_SUCCESS;
	}

	inline VkResult createLogicalDevice() {
		VkResult vkResult{};

		// Create logical device
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating logical device... ");
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
		indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		indexingFeatures.runtimeDescriptorArray = VK_TRUE;
		indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

		VkPhysicalDeviceVulkan13Features features13{};
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.shaderDemoteToHelperInvocation = VK_TRUE;

		// Chain features together
		features13.pNext = &indexingFeatures;

		// Wrap in PhysicalDeviceFeatures2 to query support
		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &features13;

		// Query supported features from the physical device
		vkGetPhysicalDeviceFeatures2(m_phyDevice, &features2);

		const char* deviceExtensions[] = { 
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			"VK_EXT_descriptor_indexing"
		};

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = std::size(deviceExtensions);
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
		deviceCreateInfo.pNext = &features2;

		Vk_CHECK(vkResult, vkCreateDevice(m_phyDevice, &deviceCreateInfo, nullptr, &m_vkDevice));
		vkGetDeviceQueue(m_vkDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


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
		depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;//VK_FORMAT_D32_SFLOAT;
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

		VkRenderPass renderPass;
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		Vk_CHECK(vkResult, vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &renderPass));

		rendPasses.push_back(renderPass);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	inline VkResult createPipelineFromMaterial(IShaderProvider* const provider, const Material& material)
	{
		LOGLINE(LogType::Info, LogMod::Vulkan, std::string{ "Creating Pipeline for " + material.name()});
		VkResult vkResult{};

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
			VK_DYNAMIC_STATE_SCISSOR
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


		// Shader Stages
		auto vs = provider->getShader(material.vShaderName);
		auto ps = provider->getShader(material.pShaderName);
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

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


		// VS input description
		std::vector<VkVertexInputAttributeDescription> attributeDescs;
		uint32_t vsIaStride = 0;
		uint32_t iaLoc = 0;
		for (const auto& attr : vs->input.attributes) {
			VkVertexInputAttributeDescription desc{};
			desc.location = iaLoc++;//attr.location != UINT8_INVALID ? attr.location : 0;
			desc.binding = 0;
			desc.format = GetVkFormat(attr.type);
			desc.offset = attr.offset;
			uint32_t end = attr.offset + sizeOfTypeVar(attr.type);
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

		// Layout
		// Descriptor sets
		struct SetLayoutInfo
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			std::vector<VkDescriptorBindingFlags> bindingFlags;
			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
		};
		std::unordered_map<uint32_t, SetLayoutInfo> bindingsPerSet;
		for (const auto& param : material.params) {
			if (param.type == TypeBase::PushConst || param.type == TypeBase::PushConstStruct)
				continue;

			auto& setInfo = bindingsPerSet[param.setIndex];

			VkDescriptorBindingFlags flags = param.arrayType != MatParamArrayType::Dynamic
				? 0
				: (VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
			setInfo.bindingFlags.push_back(flags);

			VkDescriptorSetLayoutBinding binding{};
			binding.binding = param.bindingIndex;
			binding.descriptorType = param.descriptorType;
			binding.descriptorCount = static_cast<uint32_t>(param.count);
			binding.stageFlags = MatParamStageToVkShaderStageFlagBits(param.stage);
			binding.pImmutableSamplers = nullptr;

			setInfo.bindings.push_back(binding);
		}
		std::vector<VkDescriptorSetLayout> setLayouts;
		for (auto& [set, info] : bindingsPerSet) {
			info.bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			info.bindingFlagsInfo.bindingCount = static_cast<uint32_t>(info.bindingFlags.size());
			info.bindingFlagsInfo.pBindingFlags = info.bindingFlags.data();

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(info.bindings.size());
			layoutInfo.pBindings = info.bindings.data();
			layoutInfo.pNext = &info.bindingFlagsInfo;

			VkDescriptorSetLayout layout;
			Vk_CHECK(vkResult, vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &layout));
			setLayouts.push_back(layout);
		}

		// Push Constants
		std::vector<VkPushConstantRange> pushConstantRanges;
		for (const auto& param : material.params) {
			if (param.type != TypeBase::PushConstStruct)
				continue;

			VkPushConstantRange range{};
			range.offset = param.offset;
			range.size = param.size;
			range.stageFlags = MatParamStageToVkShaderStageFlagBits(param.stage);
			pushConstantRanges.push_back(range);
		}


		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		layoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkPipelineLayout pipelineLayout;
		Vk_CHECK(vkResult, vkCreatePipelineLayout(m_vkDevice, &layoutInfo, nullptr, &pipelineLayout));


		// Create the pipeline
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
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command buffer... ");
		VkResult vkResult{};

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
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

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &frameSync[i].imageAvailable));
			Vk_CHECK(vkResult, vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &frameSync[i].inFlight));
		}
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			imageRenderDone.push_back(VkSemaphore{});
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &imageRenderDone.back()));
		}
		
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

		for (auto& s : imageRenderDone)
			vkDestroySemaphore(m_vkDevice, s, nullptr);
		imageRenderDone.clear();
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

		for (auto& pipeline : pipelines)
			vkDestroyPipeline(m_vkDevice, pipeline, nullptr);
		pipelines.clear();

		for (auto& layout : pipelineLayouts)
			vkDestroyPipelineLayout(m_vkDevice, layout, nullptr);
		pipelineLayouts.clear();

		for (auto& pass : rendPasses)
			vkDestroyRenderPass(m_vkDevice, pass, nullptr);
		rendPasses.clear();

		cleanUpSwapchainOnly();

		cleanupSyncObjects();

		vkDestroyCommandPool(m_vkDevice, commandPool, nullptr);

		vkDestroyDevice(m_vkDevice, nullptr);

		vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

		vkDestroyInstance(m_vkInstance, nullptr);

		vkDestroyDescriptorSetLayout(m_vkDevice, bindlessTextureSetLayout, nullptr);

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

	inline void draw(void* rendCtx) override {

		if (isPendingExit()) {
			vkDeviceWaitIdle(m_vkDevice);
			return;
		}

		auto* ctx = static_cast<RenderContext*>(rendCtx);
		auto& frame = frameSync[currentFrame];

		// Wait for previous frame fence 
		vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

		// Acquire swapchain image
		uint32_t imageIndex = 0;
		auto acquireResult = vkAcquireNextImageKHR(
			m_vkDevice, swapchain,
			UINT64_MAX, frame.imageAvailable,
			VK_NULL_HANDLE, &imageIndex);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || pendingResize) {
			recreateSwapchain();
			return;
		}

		// Release the fence
		vkResetFences(m_vkDevice, 1, &frame.inFlight);

		//Begin command buffer
		vkResetCommandBuffer(frame.cmdBuffer, 0);
		recordCommandBuffer(frame.cmdBuffer, currentFrame);

		
		// Begin Render Pass
		VkClearValue clearColor{};
		clearColor.color.float32[0] = ctx->clearColor[0];
		clearColor.color.float32[1] = ctx->clearColor[1];
		clearColor.color.float32[2] = ctx->clearColor[2];
		clearColor.color.float32[3] = ctx->clearColor[3];
		VkClearValue stencilClear{};
		stencilClear.depthStencil.depth = 0.0f;
		VkClearValue clearValues[] = {
			clearColor,
			stencilClear
		};

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = rendPasses[ctx->renderPassIndex];
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = &clearValues[0];
		vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set Dynamic state
		VkViewport viewport{};
		viewport.x = ctx->vPortPos[0];
		viewport.y = ctx->vPortPos[1];
		viewport.width = ctx->vPortSize[0] < 0 ? static_cast<float>(swapchainExtent.width) : ctx->vPortSize[0];
		viewport.height = ctx->vPortSize[1] < 0 ? static_cast<float>(swapchainExtent.height) : ctx->vPortSize[1];
		viewport.minDepth = ctx->vPortMinDepth;
		viewport.maxDepth = ctx->vPortMaxDepth;
		vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { ctx->sciOffset[0], ctx->sciOffset[1] };
		scissor.extent = swapchainExtent;
		vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);

		// Bind Pipeline
		vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[ctx->pipelineIndex]);

		// Draw
		vkCmdDraw(frame.cmdBuffer, 3, 1, 0, 0);

		// End Render Pass
		vkCmdEndRenderPass(frame.cmdBuffer);

		// End command buffer
		if (vkEndCommandBuffer(frame.cmdBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { frame.imageAvailable};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.cmdBuffer;
		VkSemaphore signalSemaphores[] = { imageRenderDone[imageIndex]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Submit command buffer
		if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, frame.inFlight) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// Present frame
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		auto presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || pendingResize) {
			recreateSwapchain();
		}

		// rotate frame sync /semaphores
		currentFrame = (currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
	}

	inline void notifyViewResized(void* ctx, uint16_t width, uint16_t height) override {
		pendingResize = true;
	}
};
#pragma warning(pop)
#endif