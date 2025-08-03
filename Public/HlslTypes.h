#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GlobalMacros.h"


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

struct Index32
{
	uint32_t value;

	operator uint32_t() {
		return value;
	}
	bool isValid() {
		return value != UINT32_INVALID;
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
	glm::mat4x4 modelToWorld;
	uint32_t matrixIndex;
	uint32_t boneOffset;
	uint32_t boneCount;
};

struct GlobalData
{
	GlobalData() {
		view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 5.0f), // Camera position
			glm::vec3(0.0f, 0.0f, 0.0f), // Target (look at)
			glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
		);
		cameraPosition = { 0.0f, 0.0f, 5.0f, 1.0f };
		proj = glm::perspectiveRH_ZO(
			glm::radians(60.0f), // Field of view
			16.0f / 9.0f,        // Aspect ratio
			0.1f,                // Near plane
			100.0f               // Far plane
		);
		proj[1][1] *= -1;

	}
	alignas (16) glm::mat4x4 view;
	alignas (16) glm::mat4x4 proj;
	alignas (16) glm::vec4 cameraPosition;
	alignas (16) double time{ 0 };
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
	uint32_t _pad;
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