#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <cstdint>
#include <vulkan/vulkan_core.h>

class Texture2D
{
private:


public:
	void* m_pixelData = nullptr;
	uint32_t nameIndex;
	uint16_t width;
	uint16_t height;
	uint8_t mipLevels = 1;
	uint8_t depth = 1;
	uint8_t arrayLayers = 1;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	void* pixelData() { return m_pixelData; }
	void* pixelData() const { return m_pixelData; }
};


struct Texture2DHash
{
	size_t operator()(const Texture2D& t) const {
		size_t seed = 0;
		seed ^= std::hash<uint32_t>{}(t.nameIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint16_t>{}(t.width) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint16_t>{}(t.height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(t.format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

#endif