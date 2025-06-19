#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

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

	bool m_clean = true;
	VkInstance m_vkInstance;
	VkSurfaceKHR m_vkSurface;
	VkPhysicalDevice m_curDevice;
	VkPhysicalDeviceProperties m_deviceProperties;
	VkPhysicalDeviceFeatures m_deviceFeatures;
	VkDevice m_vkDevice;
	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamilyIndex;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkSurfaceFormatKHR m_surfaceFormat;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	std::vector<VkPipelineLayout> m_pipelineLayouts;
	std::vector<VkRenderPass> m_rendPasses;
	std::vector<VkPipeline> m_pipelines;
	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	VkCommandPool m_commandPool;
	std::array<FrameSync, VULKAN_MAX_FRAMES_IN_FLIGHT> m_frameSync;
	std::vector<VkSemaphore> m_imageRenderDone;
	uint8_t m_currentFrame = 0;

public:
	virtual ~VulkanContext() {
		if (!m_clean)
			cleanUp();
	}
	VulkanContext() = delete;
	inline VulkanContext(WindowSurface* const wndSurface) :
		GraphicContext(wndSurface) {}

	inline VkResult initVulkan(const VkPresentModeKHR mode, Shader& vertShader, Shader& fragShader) {
		VkResult vk{};

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
			m_frameSync[i] = FrameSync{};
		}

		Vk_CHECK(vk, createInstance());
		Vk_CHECK(vk, createSurface());
		Vk_CHECK(vk, pickPhysDevice(false));
		Vk_CHECK(vk, createLogicalDevice());
		Vk_CHECK(vk, createSwapchain(mode));
		Vk_CHECK(vk, createImageViews());
		Vk_CHECK(vk, createRenderPass());
		Vk_CHECK(vk, createGraphicsPipeline(vertShader, fragShader, m_rendPasses[0]));
		Vk_CHECK(vk, createFramebuffers(m_rendPasses[0]));
		Vk_CHECK(vk, createCommandPool());
		Vk_CHECK(vk, createCommandBuffer());
		Vk_CHECK(vk, createSyncObjects());

		m_clean = false;

		return VK_SUCCESS;
	}

	inline VkResult createInstance() {
		VkResult vkResult;
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating VkInstance... ");
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
#else
		const char* extensions[] = {
			"VK_KHR_surface",
			"VK_KHR_win32_surface",
		};
		instanceCreateInfo.enabledExtensionCount = 2;
		instanceCreateInfo.enabledLayerCount = 0;
#endif
		instanceCreateInfo.ppEnabledExtensionNames = extensions;
		Vk_CHECK(vkResult, vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance));

		LOG(LogType::Success, "Done.");
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

		m_curDevice = VK_NULL_HANDLE;

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
					VkPhysicalDeviceProperties deviceProperties;
					VkPhysicalDeviceFeatures deviceFeatures;
					vkGetPhysicalDeviceProperties(device, &deviceProperties);
					vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

					if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
						if (prioIGpu)
							scores[d] -= 1001;
						else
							scores[d] += 1000;
					}

					if (scores[d] > bestScore) {
						bestScore = scores[d];
						m_curDevice = device;
						m_graphicsQueueFamilyIndex = i;
						m_deviceProperties = deviceProperties;
						m_deviceFeatures = deviceFeatures;
					}
				}
			}
		}

		if (m_curDevice == VK_NULL_HANDLE)
			return VK_ERROR_INITIALIZATION_FAILED;

		LOG(LogType::Remark, "Using " + std::string{ m_deviceProperties.deviceName });
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

		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		Vk_CHECK(vkResult, vkCreateDevice(m_curDevice, &deviceCreateInfo, nullptr, &m_vkDevice));

		vkGetDeviceQueue(m_vkDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createSwapchain(const VkPresentModeKHR mode) {
		VkResult vkResult;

		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Swapchain... ");

		// Query surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCaps;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_curDevice, m_vkSurface, &surfaceCaps));

		// Pick format
		uint32_t formatCount = 0;
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(m_curDevice, m_vkSurface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		Vk_CHECK(vkResult, vkGetPhysicalDeviceSurfaceFormatsKHR(m_curDevice, m_vkSurface, &formatCount, formats.data()));

		VkSurfaceFormatKHR surfaceFormat = formats[0];
		for (const auto& fmt : formats) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
				fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = fmt;
				break;
			}
		}

		m_surfaceFormat = surfaceFormat;
		m_swapchainFormat = surfaceFormat.format;
		m_swapchainExtent = { surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height };

		// Swapchain create info
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_vkSurface;
		swapchainCreateInfo.minImageCount = surfaceCaps.minImageCount + 1;
		swapchainCreateInfo.imageFormat = m_swapchainFormat;
		swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = m_swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			//| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.preTransform = surfaceCaps.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = mode;
		swapchainCreateInfo.clipped = VK_TRUE;

		Vk_CHECK(vkResult, vkCreateSwapchainKHR(m_vkDevice, &swapchainCreateInfo, nullptr, &m_swapchain));
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createImageViews() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating ImageViews... ");
		VkResult vkResult;
		// Get swapchain images
		uint32_t imageCount = 0;
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &imageCount, nullptr));
		m_swapchainImages.resize(imageCount);
		Vk_CHECK(vkResult, vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &imageCount, m_swapchainImages.data()));

		m_swapchainImageViews.resize(m_swapchainImages.size());
		for (size_t i = 0; i < m_swapchainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapchainFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			Vk_CHECK(vkResult, vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_swapchainImageViews[i]));
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
		Vk_CHECK(vkResult, vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, outShaderModule));
		return VK_SUCCESS;
	}

	inline VkResult createRenderPass() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating RenderPass... ");
		VkResult vkResult{};

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapchainFormat;
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

		Vk_CHECK(vkResult, vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &renderPass));

		m_rendPasses.push_back(renderPass);

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	inline VkResult createGraphicsPipeline(Shader& vs, Shader& ps, VkRenderPass renderPass) {
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
		viewport.width = (float)m_swapchainExtent.width;
		viewport.height = (float)m_swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchainExtent;

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
		Vk_CHECK(vkResult, vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

		m_pipelineLayouts.push_back(pipelineLayout);

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
		pipelineInfo.layout = m_pipelineLayouts.back();
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		VkPipeline pipeline;
		Vk_CHECK(vkResult, vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

		m_pipelines.push_back(pipeline);

		// clean up
		vkDestroyShaderModule(m_vkDevice, vertModule, nullptr);
		vkDestroyShaderModule(m_vkDevice, fragModule, nullptr);


		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	inline VkResult createFramebuffers(VkRenderPass renderPass) {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Framebuffers... ");
		VkResult vkResult{};

		m_swapChainFramebuffers.clear();
		m_swapChainFramebuffers.resize(m_swapchainImageViews.size());


		for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
			VkImageView attachments[] = {
				m_swapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_swapchainExtent.width;
			framebufferInfo.height = m_swapchainExtent.height;
			framebufferInfo.layers = 1;


			Vk_CHECK(vkResult, vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]));
		}

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createCommandPool() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command pool... ");
		VkResult vkResult{};

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_curDevice, m_vkSurface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		Vk_CHECK(vkResult, vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_commandPool));

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createCommandBuffer() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Command buffer... ");
		VkResult vkResult{};

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
			Vk_CHECK(vkResult, vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &m_frameSync[i].cmdBuffer));
		}
		

		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}

	inline VkResult createSyncObjects() {
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating sync objects... ");
		VkResult vkResult{};

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i) {
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_frameSync[i].imageAvailable));
			Vk_CHECK(vkResult, vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_frameSync[i].inFlight));
		}
		for (size_t i = 0; i < m_swapchainImages.size(); i++) {
			m_imageRenderDone.push_back(VkSemaphore{});
			Vk_CHECK(vkResult, vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageRenderDone.back()));
		}
		
		LOG(LogType::Success, "Done.");
		return VK_SUCCESS;
	}


	inline void cleanUp() {
		for (auto& layout : m_pipelineLayouts) 
			vkDestroyPipelineLayout(m_vkDevice, layout, nullptr);
		m_pipelineLayouts.clear();

		for (auto& pass : m_rendPasses) 
			vkDestroyRenderPass(m_vkDevice, pass, nullptr);
		m_rendPasses.clear();

		for (auto& pipeline : m_pipelines) 
			vkDestroyPipeline(m_vkDevice, pipeline, nullptr);
		m_pipelines.clear();

		for (auto& fBuffer : m_swapChainFramebuffers) 
			vkDestroyFramebuffer(m_vkDevice, fBuffer, nullptr);
		m_swapChainFramebuffers.clear();

		for (auto& sync : m_frameSync) {
			vkDestroySemaphore(m_vkDevice, sync.imageAvailable, nullptr);
			vkDestroyFence(m_vkDevice, sync.inFlight, nullptr);
		}
		for (auto& s : m_imageRenderDone)
			vkDestroySemaphore(m_vkDevice, s, nullptr);
		m_imageRenderDone.clear();
			
		vkDestroyCommandPool(m_vkDevice, m_commandPool, nullptr);
		vkDestroyDevice(m_vkDevice, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);

		m_clean = true;
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

		auto* ctx = static_cast<RenderContext*>(rendCtx);
		auto& frame = m_frameSync[m_currentFrame];

		// Wait for previous frame fence 
		vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);
		vkResetFences(m_vkDevice, 1, &frame.inFlight);

		//Begin command buffer
		vkResetCommandBuffer(frame.cmdBuffer, 0);
		recordCommandBuffer(frame.cmdBuffer, m_currentFrame);

		// Acquire swapchain image
		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(
			m_vkDevice, m_swapchain,
			UINT64_MAX, frame.imageAvailable,
			VK_NULL_HANDLE, &imageIndex);


		
		// Begin Render Pass
		VkClearValue clearColor{};
		clearColor.color.float32[0] = ctx->clearColor[0];
		clearColor.color.float32[1] = ctx->clearColor[1];
		clearColor.color.float32[2] = ctx->clearColor[2];
		clearColor.color.float32[3] = ctx->clearColor[3];

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_rendPasses[ctx->renderPassIndex];
		renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapchainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set Dynamic state
		VkViewport viewport{};
		viewport.x = ctx->vPortPos[0];
		viewport.y = ctx->vPortPos[1];
		viewport.width = ctx->vPortSize[0] < 0 ? static_cast<float>(m_swapchainExtent.width) : ctx->vPortSize[0];
		viewport.height = ctx->vPortSize[1] < 0 ? static_cast<float>(m_swapchainExtent.height) : ctx->vPortSize[1];
		viewport.minDepth = ctx->vPortMinDepth;
		viewport.maxDepth = ctx->vPortMaxDepth;
		vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { ctx->sciOffset[0], ctx->sciOffset[1] };
		scissor.extent = m_swapchainExtent;
		vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);

		// Bind Pipeline
		vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[ctx->pipelineIndex]);

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
		VkSemaphore signalSemaphores[] = { m_imageRenderDone[imageIndex]};
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

		VkSwapchainKHR swapChains[] = { m_swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

		// rotate frame sync /semaphores
		m_currentFrame = (m_currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
	}
};

#endif