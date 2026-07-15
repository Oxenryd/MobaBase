#ifndef GLFW_SURFACE_H
#define GLFW_SURFACE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "ErrorCodes.hpp"

#define Vk_FAILED(ec) ((ec) != VK_SUCCESS)
#define Vk_CHECK(ecVar, expr) (ecVar) = (expr); if (Vk_FAILED(ecVar)) return (ecVar);

class GlfwSurface
{
private:

public:
	~GlfwSurface() = default;
	GlfwSurface() = default;

	ErrorCode init() {
		if (!glfwVulkanSupported()) {
			return ErrorCode::GLFW_VULKAN_API_UNAVAILABLE;
		}

		VkResult vkResult;

		PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)
			glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
	}
};

#endif