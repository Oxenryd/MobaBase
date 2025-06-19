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

#include "WindowSurface.h"
#include "Log.hpp"

#include "GraphicContext.h"
#include "Shader.hpp"

#define Vk_FAILED(ec) ((ec) != VK_SUCCESS)
#define Vk_CHECK(ecVar, expr) (ecVar) = (expr); if (Vk_FAILED(ecVar)) return (ecVar);


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
	
	VkInstance vkInstance = nullptr;
	VkSurfaceKHR vkSurface = nullptr;
	VkPhysicalDevice curDevice = nullptr;
	//VkPhysicalDeviceProperties deviceProperties;
	//VkPhysicalDeviceFeatures deviceFeatures;
	VkDevice vkDevice = nullptr;
	VkQueue graphicsQueue = nullptr;
	uint32_t graphicsQueueFamilyIndex = static_cast<uint32_t>(-1);

	VkPresentModeKHR presentMode;
	uint32_t renderPassIndex = 0;
	bool isClean = true;
	uint8_t currentFrame = 0;
	bool pendingResize = false;
	VkSwapchainKHR swapchain = nullptr;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkSurfaceFormatKHR surfaceFormat;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

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
		curDevice{nullptr},
		//deviceFeatures{},
		frameSync{},
		graphicsQueue{nullptr},
		//deviceProperties{},
		vkInstance{nullptr},
		vkDevice{nullptr},
		graphicsQueueFamilyIndex{ static_cast<uint32_t>(-1) },
		vkSurface{nullptr},
		swapchain{nullptr},
		swapchainFormat{},
		swapchainExtent{},
		surfaceFormat{},
		presentMode{},
		renderPassIndex{}
	{}

	inline VkResult initVulkan(const VkPresentModeKHR mode, Shader& vertShader, Shader& fragShader, bool prioIGpu = false) {
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
		Vk_CHECK(vk, createGraphicsPipeline(vertShader, fragShader));
		Vk_CHECK(vk, createFramebuffers());
		Vk_CHECK(vk, createCommandPool());
		Vk_CHECK(vk, createCommandBuffer());
		Vk_CHECK(vk, createSyncObjects());

		isClean = false;

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
		Vk_CHECK(vkResult, vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance));

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

		Vk_CHECK(vkResult, vkCreateWin32SurfaceKHR(vkInstance, &surfaceCreateInfo, nullptr, &vkSurface));
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult pickPhysDevice(bool prioIGpu) {
		VkResult vkResult{};
		// Enumerate physical devices
		LOGLINE(LogType::Info, LogMod::Vulkan, "Choosing GPU... ");
		uint32_t deviceCount = 0;
		Vk_CHECK(vkResult, vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		Vk_CHECK(vkResult, vkEnumeratePhysicalDevices(vkInstance, &deviceCount, physicalDevices.data()));

		curDevice = VK_NULL_HANDLE;
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
				Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkSurface, &supportsPresent));

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
						curDevice = device;
						graphicsQueueFamilyIndex = i;
						//deviceFeatures = deviceFeatures;
					}
				}
			}
		}

		if (curDevice == VK_NULL_HANDLE)
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
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		Vk_CHECK(vkResult, vkCreateDevice(curDevice, &deviceCreateInfo, nullptr, &vkDevice));

		vkGetDeviceQueue(vkDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createSwapchain() {
		VkResult vkResult;

		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Swapchain... ");

		// Query surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCaps;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(curDevice, vkSurface, &surfaceCaps));

		// Pick format
		uint32_t formatCount = 0;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(curDevice, vkSurface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(curDevice, vkSurface, &formatCount, formats.data()));

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
		swapchainCreateInfo.surface = vkSurface;
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

		Vk_CHECK(vkResult, vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &swapchain));
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createImageViews() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating ImageViews... ");
		VkResult vkResult;
		// Get swapchain images
		uint32_t imageCount = 0;
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(vkDevice, swapchain, &imageCount, nullptr));
		swapchainImages.resize(imageCount);
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(vkDevice, swapchain, &imageCount, swapchainImages.data()));

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

			Vk_CHECK(vkResult, vkCreateImageView(vkDevice, &createInfo, nullptr, &swapchainImageViews[i]));
		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createShaderModule(Shader& shader, VkShaderModule* outShaderModule) {
		VkResult vkResult{};
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shader.bytecode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.bytecode.data());
		Vk_CHECK(vkResult, vkCreateShaderModule(vkDevice, &createInfo, nullptr, outShaderModule));
		return VK_SUCCESS;
	}

	inline VkResult createRenderPass() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating RenderPass... ");
		VkResult vkResult{};

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPass renderPass;
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		Vk_CHECK(vkResult, vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &renderPass));

		rendPasses.push_back(renderPass);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	inline VkResult createGraphicsPipeline(Shader& vs, Shader& ps) {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Pipeline... ");
		VkResult vkResult{};
		VkShaderModule vertModule{};
		Vk_CHECK(vkResult, createShaderModule(vs, &vertModule));
		VkShaderModule fragModule{};
		Vk_CHECK(vkResult, createShaderModule(ps, &fragModule));

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional to be filled
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional to be filled

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

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

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		rasterizer.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		VkPipelineLayout pipelineLayout;
		Vk_CHECK(vkResult, vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

		pipelineLayouts.push_back(pipelineLayout);

		// The Sauce!
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayouts.back();
		pipelineInfo.renderPass = rendPasses[renderPassIndex];
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		VkPipeline pipeline;
		Vk_CHECK(vkResult, vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

		pipelines.push_back(pipeline);

		// clean up
		vkDestroyShaderModule(vkDevice, vertModule, nullptr);
		vkDestroyShaderModule(vkDevice, fragModule, nullptr);


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
				swapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = rendPasses[renderPassIndex];
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapchainExtent.width;
			framebufferInfo.height = swapchainExtent.height;
			framebufferInfo.layers = 1;

			Vk_CHECK(vkResult, vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createCommandPool() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command pool... ");
		VkResult vkResult{};

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(curDevice, vkSurface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		Vk_CHECK(vkResult, vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &commandPool));

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
			Vk_CHECK(vkResult, vkAllocateCommandBuffers(vkDevice, &allocInfo, &frameSync[i].cmdBuffer));
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
			Vk_CHECK(vkResult, vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &frameSync[i].imageAvailable));
			Vk_CHECK(vkResult, vkCreateFence(vkDevice, &fenceInfo, nullptr, &frameSync[i].inFlight));
		}
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			imageRenderDone.push_back(VkSemaphore{});
			Vk_CHECK(vkResult, vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageRenderDone.back()));
		}
		
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult recreateSwapchain() {
		VkResult vkResult{};

		LOGLINE_IND(LogType::Info, LogMod::Vulkan, "Recreating Swapchain... ", 1);

		vkDeviceWaitIdle(vkDevice);

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
			vkDestroyFramebuffer(vkDevice, swapChainFramebuffers[i], nullptr);
		}
		swapChainFramebuffers.clear();

		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			vkDestroyImageView(vkDevice, swapchainImageViews[i], nullptr);
		}
		swapchainImageViews.clear();

		vkDestroySwapchainKHR(vkDevice, swapchain, nullptr);
	}

	inline void cleanupSyncObjects() {
		for (auto& sync : frameSync) {
			vkDestroySemaphore(vkDevice, sync.imageAvailable, nullptr);
			vkDestroyFence(vkDevice, sync.inFlight, nullptr);
		}

		for (auto& s : imageRenderDone)
			vkDestroySemaphore(vkDevice, s, nullptr);
		imageRenderDone.clear();
	}

	inline void cleanUp() {

		LOGLINE(LogType::Info, LogMod::Vulkan, "Cleaning up... ");

		vkDeviceWaitIdle(vkDevice);

		for (auto& pipeline : pipelines)
			vkDestroyPipeline(vkDevice, pipeline, nullptr);
		pipelines.clear();

		for (auto& layout : pipelineLayouts)
			vkDestroyPipelineLayout(vkDevice, layout, nullptr);
		pipelineLayouts.clear();

		for (auto& pass : rendPasses)
			vkDestroyRenderPass(vkDevice, pass, nullptr);
		rendPasses.clear();

		cleanUpSwapchainOnly();

		cleanupSyncObjects();

		vkDestroyCommandPool(vkDevice, commandPool, nullptr);

		vkDestroyDevice(vkDevice, nullptr);

		vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

		vkDestroyInstance(vkInstance, nullptr);

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
			vkDeviceWaitIdle(vkDevice);
			return;
		}

		auto* ctx = static_cast<RenderContext*>(rendCtx);
		auto& frame = frameSync[currentFrame];

		// Wait for previous frame fence 
		vkWaitForFences(vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

		// Acquire swapchain image
		uint32_t imageIndex = 0;
		auto acquireResult = vkAcquireNextImageKHR(
			vkDevice, swapchain,
			UINT64_MAX, frame.imageAvailable,
			VK_NULL_HANDLE, &imageIndex);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || pendingResize) {
			recreateSwapchain();
			return;
		}

		// Release the fence
		vkResetFences(vkDevice, 1, &frame.inFlight);

		//Begin command buffer
		vkResetCommandBuffer(frame.cmdBuffer, 0);
		recordCommandBuffer(frame.cmdBuffer, currentFrame);

		
		// Begin Render Pass
		VkClearValue clearColor{};
		clearColor.color.float32[0] = ctx->clearColor[0];
		clearColor.color.float32[1] = ctx->clearColor[1];
		clearColor.color.float32[2] = ctx->clearColor[2];
		clearColor.color.float32[3] = ctx->clearColor[3];

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = rendPasses[ctx->renderPassIndex];
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
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
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.inFlight) != VK_SUCCESS)
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
		auto presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
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