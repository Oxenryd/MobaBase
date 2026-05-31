#ifndef HLSLTYPES_H
#define HLSLTYPES_H

#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <array>
#include <span>


#include "GlobalMacros.h"
#include "Concepts.h"

template <typename T, int N>
struct FloatArray
{
private:
	T m_data[N];

public:
	T& operator[](int index) { return m_data[index]; }

	template<IsGlmVecCompatible<N> V>
	FloatArray& operator=(const V& vec) {
		std::memcpy(m_data, &vec, sizeof(T) * N);
		return *this;
	}

	const glm::vec<N, float>& asGlmVec() const {
		if constexpr (N == 2)
			return reinterpret_cast<const glm::vec2&>(*m_data);//glm::vec2{m_data[0], m_data[1]};
		if constexpr (N == 3)
			return reinterpret_cast<const glm::vec3&>(*m_data);//glm::vec3{m_data[0], m_data[1], m_data[2]};
		if constexpr (N == 4)
			return reinterpret_cast<const glm::vec4&>(*m_data);//glm::vec4{m_data[0], m_data[1], m_data[2], m_data[3]};

		if constexpr (N < 2 || N > 4)
			static_assert(false, "No valid GLM Vec conversion.");

	}

	T* data() { return m_data; }
	T* data() const { return m_data; }

	explicit operator const T*() { return m_data; }
	explicit operator std::array<T, N>&()  { return m_data; }
	explicit operator std::span<T>() { return std::span<T>{m_data, N}; }
};

typedef FloatArray<float, 3> FArray3;
typedef FloatArray<float, 4> FArray4;

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

#ifdef BUILD_WIN
#define LIGHT_CONST inline static
#else
#define LIGHT_CONST constexpr
#endif


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

struct alignas (64) ModelTransform
{
	glm::mat4x4 mtw;
	glm::mat4x4 invT;

	~ModelTransform() = default;
	ModelTransform(const ModelTransform&& other) = delete;
	ModelTransform& operator=(const ModelTransform&& other) = delete;

	explicit ModelTransform(const glm::mat4x4& mat) :
		mtw{mat}
	{
		invT = glm::transpose(glm::inverse(mtw));
	}

	ModelTransform(const ModelTransform& other) :
		mtw{ other.mtw }, invT{other.invT} {}

	ModelTransform& operator=(const ModelTransform& other) {
		mtw = other.mtw;
		invT = other.invT;
		return *this;
	}

	ModelTransform& operator=(const glm::mat4x4& mat) {
		mtw = mat;
		invT = glm::transpose(glm::inverse(mtw));
		return *this;
	}

	explicit operator glm::mat4x4() const& {
		return mtw;
	}
	glm::mat4x4 operator*(const glm::mat4x4& rhs) const { return mtw * rhs; }
	glm::mat4x4& operator*=(const glm::mat4x4& rhs) {
		mtw = mtw * rhs;
		invT = glm::transpose(glm::inverse(mtw));
		return mtw;
	}
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
	glm::mat4x4 invT {1};
	uint32_t flags{ 0 };
	uint32_t matInstanceIndex{ UINT32_INVALID };
	uint32_t boneOffset{ UINT32_INVALID };
	uint32_t boneCount{ 0 };
};

struct alignas (16) CameraData
{
	CameraData() {
		vFov = 60.f;
		nearPlane = 0.1f;
		farPlane = 1000.0f;
		aspectRatio = 16.0f / 9.0f;
		cameraPosition[0] = 0.0f;
		cameraPosition[1] = 0.0f;
		cameraPosition[2] = 5.0f;
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

	union alignas (16)
	{
		float camPos_amb[4];
		struct
		{
			float cameraPosition[3];
			float ambient;
		};
	};

	union alignas (16)
	{
		float screenSizes[4];
		struct
		{
			float screenSize[2];
			float invScreenSize[2];
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
	uint32_t flags = 0;
};

struct alignas (16) BaseMaterialInstance
{
    TexturePack textures;
	glm::vec3	ambient{1.0f, 1.0f, 1.0f};
	float		ambientIntensity{ 0.05f};
    
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

	void setTextureFlags(uint32_t flags);
};

#endif