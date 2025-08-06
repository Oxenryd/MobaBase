#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GlobalMacros.h"

//using BindSetPair = std::pair<uint8_t, uint8_t>;

struct BindSetCombo
{
	~BindSetCombo() = default;
	BindSetCombo() = default;
	BindSetCombo(const BindSetCombo& other) = default;
	BindSetCombo& operator=(const BindSetCombo& rhs) = default;
	BindSetCombo(const uint8_t bind, const uint8_t set) :
		binding{ bind }, set{ set }, layoutHandle{ 0 } {}
	BindSetCombo(const uint8_t bind, const uint8_t set, const uint64_t layoutHandle) :
		binding{ bind }, set{ set }, layoutHandle{ layoutHandle } {}
	uint8_t binding{ UINT8_INVALID };
	uint8_t set{ UINT8_INVALID };
	uint64_t layoutHandle{ UINT64_INVALID };

	bool operator==(const BindSetCombo& rhs) const {
		return
			binding == rhs.binding &&
			set == rhs.set &&
			layoutHandle == rhs.layoutHandle;
	}
};

struct BindSetComboKeyHash
{
	size_t operator()(const BindSetCombo& key) const {
		uint64_t seed = 0;
		seed ^= std::hash<uint64_t>{}(static_cast<uint64_t>(key.binding))+0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(static_cast<uint64_t>(key.set)) +0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(static_cast<uint64_t>(key.layoutHandle)) +0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

struct BaseVSIn
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;
	glm::vec2 texCoord;
	//uint32_t instanceID;
};

struct ModelTransform
{
	ModelTransform(const glm::mat4x4& mat) :
		modelToWorld{mat} {}

	ModelTransform& operator=(const glm::mat4x4& mat) {
		modelToWorld = mat;
		return *this;
	}

	glm::mat4x4 modelToWorld;

	operator glm::mat4x4()& {
		return modelToWorld;
	}
	glm::mat4x4 operator*(glm::mat4x4& rhs) const { return modelToWorld * rhs; }
	glm::mat4x4& operator*=(glm::mat4x4& rhs) { return modelToWorld = modelToWorld * rhs; }
};

struct InstanceData
{
	uint32_t matrixIndex{ UINT32_INVALID };
	uint32_t matInstanceIndex{ UINT32_INVALID };
	uint32_t boneOffset{ UINT32_INVALID };
	uint32_t boneCount{ 0 };

	operator uint32_t() {
		return matInstanceIndex;
	}
	bool isValid() {
		return matInstanceIndex != UINT32_INVALID;
	}
};

struct VSInput
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 tangent;
	glm::vec4 color;
	glm::u32vec4 boneIndices;
	glm::vec4 boneWeights;
};

struct VSOuput
{
	glm::vec4 worldPos;
	glm::vec3 localPos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct BaseMatPush
{
	glm::mat4x4 modelToWorld{ 1 };
	uint32_t matrixIndex{ UINT32_INVALID };
	uint32_t matInstanceIndex{ UINT32_INVALID };
	uint32_t boneOffset{ UINT32_INVALID };
	uint32_t boneCount{ 0 };
};

struct CameraData
{
	CameraData() {
		vFov = 60.f;
		nearPlane = 0.1f;
		farPlane = 1000.0f;
		aspectRatio = 16.0f / 9.0f;
		cameraPosition = { 0.0f, 0.0f, 5.0f, 1.0f };
		view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 5.0f), // Camera position
			glm::vec3(0.0f, 0.0f, 0.0f), // Target (look at)
			glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
		);
		
		proj = glm::perspectiveRH_ZO(
			glm::radians(vFov),
			aspectRatio,
			nearPlane,
			farPlane
		);
		proj[1][1] *= -1;

	}
	CameraData(const CameraData& other) = default;

	alignas (16) glm::mat4x4 view;
	alignas (16) glm::mat4x4 proj;
	alignas (16) glm::vec4 cameraPosition;
	float vFov;
	float nearPlane;
	float farPlane;
	float aspectRatio;
};

struct alignas (16) SpriteInstance
{
	glm::vec2 position;
	glm::vec2 size;
	float origin[2];
	float rotation;
	float layerDepth;
	uint32_t texRect[4];
	uint32_t texIndex;
	uint32_t _pad;
};

struct alignas (16) TexturePack
{
	uint32_t albedoId = UINT32_INVALID;
	uint32_t normalId = UINT32_INVALID;
	uint32_t specularId = UINT32_INVALID;
	uint32_t roughnessId = UINT32_INVALID;
	uint32_t emissiveId = UINT32_INVALID;
	uint32_t metallicId = UINT32_INVALID;
	uint32_t aoId = UINT32_INVALID;
	uint32_t _pad0 = UINT32_INVALID;
};

struct alignas (16) BaseMaterialInstance
{
    TexturePack textures;
	glm::vec3	ambient{1, 1, 1};
	float		ambientIntensity{ 1 };
    
	glm::vec3	baseColor{ 1, 0, 1 };
	float		albedoStrength{ 1 };
    
	glm::vec3	specular{ 0, 0, 0 };
	float		specularStrength{ 1 };
    
	glm::vec3	emissive{ 0.05f, 0.05f, 0.05f };
	float		shininess{ 0.15f };
    
	float		refraction{ 1.5f };
	float		transparency{ 1.0f };
	float		roughness{ 0.05f };
	float		metallic{ 0 };
    
	glm::vec3	transparentColor{ 1, 1, 1 };
	float		ao{ 0 };

	float		reflectivity{ 0 };
	float		transmission{ 0 };
	float		emissiveStrength{ 0.15f };
	float		clearcoatStrength{ 0 };
};

#endif