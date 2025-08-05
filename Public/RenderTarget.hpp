#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP

#include <vulkan/vulkan_core.h>

struct VkTextureResource
{
	VkDescriptorImageInfo imageInfo;
	VkImageView imageView;
	VkImage image;
	VkDeviceMemory memory;
};

struct RenderTarget
{
	VkTextureResource resource;
	VkExtent2D extent;
	VkFramebuffer framebuffer;
};


#endif