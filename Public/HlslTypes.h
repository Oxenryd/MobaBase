#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <glm/glm.hpp>
#include "GlobalMacros.h"


struct BaseVSIn
{
	alignas (16) glm::vec3 pos;
	alignas (16) glm::vec3 normal;
	alignas (16) glm::vec3 tangent;
	alignas (16) glm::vec3 binormal;
	alignas (16) glm::vec2 texCoord;
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

struct ObjectPush
{
	alignas (16) glm::mat4x4 model;
	alignas (4) uint32_t g_boneOffset;
	alignas (4) uint32_t g_boneCount;
	alignas (8) uint64_t _pad;
};

struct GlobalData
{
	alignas (16) glm::mat4x4 view;
	alignas (16) glm::mat4x4 proj;
	alignas (16) glm::vec4 cameraPosition;
	alignas (16) float time;
};

struct alignas (16) SpriteInstance
{
	float position[2];
	float size[2];
	float origin[2];
	float rotation;
	float layerDepth;
	uint32_t texRect[4];
	uint32_t texIndex;
	uint32_t _pad;
};

struct TexturePack
{
	uint32_t albedoId = UINT32_INVALID;
	uint32_t normalId = UINT32_INVALID;
	uint32_t specularId = UINT32_INVALID;
	uint32_t roughnessId = UINT32_INVALID;
};

struct BaseMaterialInstance
{
	TexturePack textures;
	glm::vec3	ia;
	float		ka;
	glm::vec3	id;
	float		kd;
	glm::vec3	is;
	float		ks;
};

#endif