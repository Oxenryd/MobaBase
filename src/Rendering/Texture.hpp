#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <array>

#include "ErrorCodes.hpp"
#include "GlobalMacros.h"
#include <assimp/material.h>

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

enum class TextureType : uint8_t
{
	Unknown = 0,
	Diffuse,
	Normal,
	Specular,
	Metalness,
	Roughness,
	Emissive,
	Height,
	AmbientOcclusion,
	Clearcoat
};


class Texture2D
{

public:
	~Texture2D() = default;
	Texture2D() = default;
	Texture2D(const Texture2D& other) = default;
	Texture2D& operator=(const Texture2D& rhs) = default;
	Texture2D(Texture2D&& other) = default;
	Texture2D& operator=(Texture2D&& other) = default;

	std::string filePath;
	uint32_t textureIndex = UINT32_INVALID;
	uint16_t width{ 0 };
	uint16_t height{ 0 };
	//uint8_t mipLevels = 4;
	bool generateMips = true;
	uint8_t depth = 1;
	uint8_t arrayLayers = 1;
	TextureType type = TextureType::Unknown;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	uint32_t resourceIndex = UINT32_INVALID;
	union
	{
		void* texelPtr;
		size_t texelOffset = UINT32_INVALID;
	};
	

	void* texelData();
	[[nodiscard]] void* texelData() const;
	[[nodiscard]] size_t texelCount() const { return width * height; }
	VkTextureResource* getResource();
	ErrorCode tryAllocate();

	bool operator==(const Texture2D& rhs);

	static TextureType typeFrom_aiTextureType(aiTextureType aiType);
	static constexpr std::array<aiTextureType, 10> aiTypesList = {
		aiTextureType_BASE_COLOR,
		aiTextureType_DIFFUSE,
		aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT,
		aiTextureType_CLEARCOAT,
		aiTextureType_METALNESS,
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_NORMALS,
		aiTextureType_SPECULAR,
		aiTextureType_DIFFUSE_ROUGHNESS
	};

	void exportPNG(const std::string& filepath);
};


struct Texture2DHash
{
	static size_t hash(const Texture2D& t) {
		size_t seed = 0;
		seed ^= std::hash<uint32_t>{}(t.textureIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint16_t>{}(t.width) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint16_t>{}(t.height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(t.format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(static_cast<uint64_t>(t.type)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}

	size_t operator()(const Texture2D& t) const {
		return hash(t);
	}
};



#endif