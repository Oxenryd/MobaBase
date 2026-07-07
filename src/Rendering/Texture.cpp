#include "Texture.hpp"
#include "Engine.h"
#include "RenderManager.h"
#include "VulkanContext.hpp"

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
	#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "stb_image_write.h"

void* Texture2D::texelData() {
	if (texelCount() == 0)
		return nullptr;
	if (texelOffset == UINT32_INVALID)
		return nullptr;
	if (filePath.empty())
		return texelPtr;

	return &Engine::getInstance()->getRenderManager()->texels()[texelOffset];
}

void* Texture2D::texelData() const {
	if (texelCount() == 0) 
		return nullptr;
	if (texelOffset == UINT32_INVALID)
		return nullptr;
	if (filePath.empty())
		return texelPtr;

	return &Engine::getInstance()->getRenderManager()->texels()[texelOffset];
}

VkTextureResource* Texture2D::getResource() {
	return nullptr;
}

ErrorCode Texture2D::tryAllocate() {

	if (resourceIndex != UINT32_INVALID)
		return ErrorCode::TEXTURE_ALREADY_ALLOCATED;

	auto resource = RenderManager::getInstance()->vkContext()->getTexResource(resourceIndex);
	if (resource && resource->memory != VK_NULL_HANDLE)
		return ErrorCode::TEXTURE_ALREADY_ALLOCATED;
	else if (!resource) {
		RenderManager::getInstance()->vkContext()->loadTexture(*this, &resourceIndex);
	}
	

	return ErrorCode::OK;
}

bool Texture2D::operator==(const Texture2D& rhs) {
	return
		filePath == rhs.filePath && Texture2DHash::hash(*this) == Texture2DHash::hash(rhs);

}

TextureType Texture2D::typeFrom_aiTextureType(aiTextureType aiType) {
	
	switch (aiType) {
		case aiTextureType::aiTextureType_BASE_COLOR:
		case aiTextureType::aiTextureType_DIFFUSE:
			return TextureType::Diffuse;

		case aiTextureType::aiTextureType_EMISSIVE:
			return TextureType::Emissive;

		case aiTextureType::aiTextureType_HEIGHT:
			return TextureType::Height;

		case aiTextureType::aiTextureType_CLEARCOAT:
			return TextureType::Clearcoat;

		case aiTextureType::aiTextureType_METALNESS:
			return TextureType::Metalness;

		case aiTextureType::aiTextureType_AMBIENT_OCCLUSION:
			return TextureType::AmbientOcclusion;

		case aiTextureType::aiTextureType_NORMALS:
			return TextureType::Normal;

		case aiTextureType::aiTextureType_SPECULAR:
			return TextureType::Specular;

		case aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS:
			return TextureType::Roughness;

		default:
			return TextureType::Unknown;
	}
}

void Texture2D::exportPNG(const std::string& filepath) {

	if (!texelData() || width == 0 || height == 0) {
		LOGLINE(LogType::Warning, LogMod::Assets, "exportPNG: No texture data available for export.. ");
		return;
	}

	int channels = 4; // Assuming VK_FORMAT_R8G8B8A8_UNORM
	if (stbi_write_png(filepath.c_str(),
					   width,
					   height,
					   channels,
					   texelData(),
					   width * channels) == 0) {
		LOGLINE(LogType::Error, LogMod::Assets, std::format("exportPNG: failed to write '{}'.. ", filepath));
	} else {
		LOGLINE(LogType::Success, LogMod::Assets, std::format("exportPNG: Saved '{}'. ", filepath));
	}
}
