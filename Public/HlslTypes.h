#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <glm/glm.hpp>

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

#endif