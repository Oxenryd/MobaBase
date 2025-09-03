#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "GlobalMacros.h"

struct ShapeRendAABB
{
	glm::vec3 mn;
	glm::vec3 mx;
};

struct ShapePush
{
	glm::mat4x4 modelToWorld;
	glm::vec4 color;
	ShapeRendAABB aabb;
	glm::quat rotation;
	uint32_t drawNumber;
};


enum class LightType : uint8_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	_ENUM_END
};

constexpr static size_t NUM_OF_LIGHT_TYPES = (size_t)LightType::_ENUM_END;

//enum LightType : uint32_t
//{
//	CastShadow = 1 << 0,
//	// room for more: Static, Volumetric, ContactShadows...
//};

enum class ShadowType : uint32_t
{
	None,
	DirCSM,
	Spot2D,
	PointCube
};

enum class LightFlags : uint32_t
{
	None = 0x00000000,
	Enabled = 0x00000001,
	Static = 0x00000002
};

INLINE auto operator|(LightFlags a, LightFlags b) {
	return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

struct alignas(16) GPULight
{
	// 0..15
	union
	{
		glm::vec4 positionVS_radius{ 0.0f, 0.0f, 0.0f, 1.0f };
		struct
		{
			glm::vec3 positionVS;
			float radius;
		};
	};


	// 16..31
	union
	{
		glm::vec4 directionVS_spotInnerCos{ 0.0f, 0.0f, -1.0f, 1.0f };
		struct
		{
			glm::vec3 directionVS;
			float spotInnerCos;
		};
	};


	// 32..47
	union
	{
		glm::vec4 color_intensity{ 1.0f, 1.0f, 1.0f, 1.0f };
		struct
		{
			glm::vec3 color;
			float intensity;
		};
	};


	// 48..63
	uint32_t type{ (uint32_t)LightType::Directional };
	uint32_t flags{ (uint32_t)(LightFlags::Enabled | LightFlags::Static)};
	uint32_t cookieIndex{ UINT32_INVALID };
	uint32_t shadowIndex{ UINT32_INVALID };

	// 64..79
	float spotOuterCos{ 1.0f };
	float falloffExp{ 1.5f };
	float invRange{ 1.0f };
	float volumetricIntensity{ 1.0f };

	// 80..95
	float volumetricFalloff{ 1.0f };
	float shadowBias{ 0.0f };
	float shadowNormalBias{ 0.0f };
	float _reserved0{ 0 };

	// 96..111
	uint32_t shadowType{ 0 };
	uint32_t shadowLayerCount{ 0 };
	uint32_t _reserved1{ 0 };;
	uint32_t _reserved2{ 0 };;

};

static_assert(sizeof(GPULight) == 112, "GPULight must be 112 bytes");


namespace LightFactory
{
	static const GPULight Point(
		const glm::vec3& pos,
		float radius,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		float intensity = 1.0f) {
		GPULight L{};
		L.type = static_cast<uint32_t>(LightType::Point);
		L.positionVS = pos;
		L.radius = radius;
		L.invRange = radius > 0.0f ? 1.0f / radius : 0.0f;
		L.color = color;
		L.intensity = intensity;
		return L;
	}

	static const GPULight Spot(
		const glm::vec3& pos,
		const glm::vec3& dir,
		float radius,
		float innerDeg,
		float outerDeg,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		float intensity = 1.0f) {
		GPULight L{};
		L.type = static_cast<uint32_t>(LightType::Spot);
		L.positionVS = pos;
		L.directionVS = glm::normalize(dir);
		L.radius = radius;
		L.invRange = radius > 0.0f ? 1.0f / radius : 0.0f;
		L.spotInnerCos = glm::cos(glm::radians(innerDeg));
		L.spotOuterCos = glm::cos(glm::radians(outerDeg));
		L.color = color;
		L.intensity = intensity;
		return L;
	}

	static const GPULight Directional(
		const glm::vec3& dir,
		const glm::vec3& color = { 1.0f, 1.0f, 1.0f },
		float intensity = 1.0f) {
		GPULight L{};
		L.type = static_cast<uint32_t>(LightType::Directional);
		L.directionVS = glm::normalize(dir);
		L.radius = 0.0f;       // Infinite range
		L.invRange = 0.0f;
		L.color = color;
		L.intensity = intensity;
		return L;
	}
};


struct ShadowMatrix { glm::mat4 viewProj; };



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

struct Index32
{
private:
	uint32_t m_value;
public:
	Index32() : m_value{0} {}
	Index32(const Index32& value) :
		m_value { value.m_value } {}

	Index32& operator=(const Index32& other) {
		m_value = other.m_value;
		return *this; }
	Index32(const uint32_t& value) :
		m_value{ value} {}

	Index32& operator=(const uint32_t& other) {
		m_value = other;
		return *this; }

	operator uint32_t&() { return m_value; }

	uint32_t operator* (const uint32_t& rhs) const { return m_value * rhs; }
	uint32_t operator/ (const uint32_t& rhs) const { return m_value / rhs; }
	uint32_t operator+ (const uint32_t& rhs) const { return m_value + rhs; }
	uint32_t operator- (const uint32_t& rhs) const { return m_value - rhs; }

	uint32_t& operator*= (const uint32_t& rhs) { return m_value = m_value * rhs; }
	uint32_t& operator+= (const uint32_t& rhs) { return m_value = m_value + rhs; }
	uint32_t& operator-= (const uint32_t& rhs) { return m_value = m_value - rhs; }
	uint32_t& operator/= (const uint32_t& rhs) { return m_value = m_value / rhs; }

	uint32_t* data() { return &m_value; }
};


struct Index64
{
	uint32_t x;
	uint32_t y;
};

struct Index96
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
};

struct Index128
{
	union
	{
		Index32 value[4];
		struct
		{
			uint32_t x, y, z, w;
		};
	};
	
	Index128(const Index128& value) {
		std::memcpy(&this->value, &value.value, 4 * sizeof(Index32));
	}

	Index128& operator=(const Index128& other) {
		std::memcpy(&value, &other.value, 4 * sizeof(Index32));
		return *this;
	}
	Index128(const uint32_t* value) {
		std::memcpy(&this->value, value, 4 * sizeof(Index32));
	}
	Index128& operator=(const uint32_t* other) {
		std::memcpy(&value, other, 4 * sizeof(Index32));
		return *this;
	}


	uint32_t* data() { return reinterpret_cast<uint32_t*>(&value); }
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

//struct VSInput
//{
//	glm::vec3 position;
//	glm::vec3 normal;
//	glm::vec2 uv;
//	glm::vec4 tangent;
//	glm::vec4 color;
//	glm::u32vec4 boneIndices;
//	glm::vec4 boneWeights;
//};
//
//struct VSOuput
//{
//	glm::vec4 worldPos;
//	glm::vec3 localPos;
//	glm::vec3 normal;
//	glm::vec2 uv;
//};

enum class BaseMatPushFlags : uint32_t
{
	None		= 0x00000000,
	Instanced	= 0x00000001
};

struct BaseMatPush
{
	glm::mat4x4 modelToWorld{ 1 };
	uint32_t flags{ 0 };
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
		cameraPosition = { 0.0f, 0.0f, 5.0f};
		ambient = 0.00625f;
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
		
		invProj = 1.0f / proj;

		clustersX = VULKAN_LIGHT_CLUSTERS_X;
		clustersY = VULKAN_LIGHT_CLUSTERS_Y;
		clustersZ = VULKAN_LIGHT_CLUSTERS_Z;

	}
	CameraData(const CameraData& other) = default;

	alignas (16) glm::mat4x4 view;

	alignas (16) glm::mat4x4 proj;

	alignas (16) glm::mat4x4 invProj;

	union
	{
		alignas (16) glm::vec4 camPos_amb;
		struct
		{
			glm::vec3 cameraPosition;
			float ambient;
		};
	};

	union
	{
		alignas (16) glm::vec4 screenSizes{};
		struct
		{
			glm::vec2 screenSize;
			glm::vec2 invScreenSize;
		};
	};
	

	float vFov;
	float nearPlane;
	float farPlane;
	float aspectRatio;

	uint32_t numLights{};
	uint32_t clustersX{}, clustersY{}, clustersZ{};

	float clusterNearK{};
	float clusterLogBase{};
	float _pad0{};
	float _pad1{};
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
    
	glm::vec3	baseColor{ 1, 1, 1 };
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