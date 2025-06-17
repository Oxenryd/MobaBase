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

#include "WindowSurface.h"
#include "Log.hpp"

#include "GraphicContext.h"

#define Vk_FAILED(ec) ((ec) != VK_SUCCESS)
#define Vk_CHECK(ecVar, expr) (ecVar) = (expr); if (Vk_FAILED(ecVar)) return (ecVar);


class VulkanContext : public GraphicContext
{
private:
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
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

public:
	~VulkanContext() {}
	VulkanContext() = delete;
	inline VulkanContext(WindowSurface* const wndSurface) :
		GraphicContext(wndSurface) {}

	inline VkResult create(WindowSurface* wndSurface) {
		VkResult vkResult;
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating VkInstance... ");
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = wndSurface->appName.c_str();

		// Create instance
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		const char* extensions[] = { 
			"VK_KHR_surface",
			"VK_KHR_win32_surface"
#ifdef VULKAN_VALIDATION
			, "VK_EXT_debug_utils"
#endif
		};
		instanceCreateInfo.enabledExtensionCount = 3;
		instanceCreateInfo.ppEnabledExtensionNames = extensions;
#ifdef VULKAN_VALIDATION
		if (checkValidationLayerSupport()) {
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			instanceCreateInfo.enabledLayerCount = 0;
		}
#endif
		Vk_CHECK(vkResult, vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance));

		LOG(LogType::Success, "Done.");

		// Create surface
		LOGLINE(LogType::Info, LogMod::Vulkan, "Creating Vk_Win32 Surface... ");
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = wndSurface->windowInstance;
		surfaceCreateInfo.hwnd = wndSurface->windowHandle;

		Vk_CHECK(vkResult, vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_vkSurface));
		LOG(LogType::Success, "Done.");


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
					if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						scores[d] += 1000;
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

	inline VkResult createSwapchain(WindowSurface* wndSurface, const VkPresentModeKHR mode) {
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
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB) {
				surfaceFormat = fmt;
				break;
			}
		}

		m_swapchainFormat = surfaceFormat.format;
		m_swapchainExtent = { surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height };

		// Swapchain create info
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_vkSurface;
		swapchainCreateInfo.minImageCount = surfaceCaps.minImageCount + 1;
		swapchainCreateInfo.imageFormat = m_swapchainFormat;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = m_swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.preTransform = surfaceCaps.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = mode;
		swapchainCreateInfo.clipped = VK_TRUE;

		Vk_CHECK(vkResult, vkCreateSwapchainKHR(m_vkDevice, &swapchainCreateInfo, nullptr, &m_swapchain));

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

	inline VkResult createGraphicsPipeline() {

	}

	inline void cleanUp() {
		vkDestroyDevice(m_vkDevice, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);
	}
};

#endif