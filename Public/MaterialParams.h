#ifndef MATERIALPARAMS_H
#define MATERIALPARAMS_H

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "Texture.hpp"
#include "TypeReflection.hpp"
#include "Log.hpp"

using json = nlohmann::json;

enum class MatParamStage : uint8_t
{
	None,
	Vertex,
	Fragment,
	Both
};

enum class MatParamArrayType : uint8_t
{
	None,
	Static,
	Dynamic
};



inline static constexpr const char* MatParamStageToString(MatParamStage stage) {
	switch (stage) {
		case MatParamStage::Vertex:   return "Vertex";
		case MatParamStage::Fragment: return "Fragment";
		case MatParamStage::Both:     return "Both";
		default:                      return "Unknown";
	}
}

inline constexpr MatParamStage MatParamStageFromString(const std::string& s) {
	if (s == "Vertex")   return MatParamStage::Vertex;
	if (s == "Fragment") return MatParamStage::Fragment;
	if (s == "Both")     return MatParamStage::Both;
	throw std::invalid_argument("Invalid MatParamStage: " + s);
}



struct MatVar
{
	union
	{

		uint64_t _raw;
		struct
		{
			uint64_t _type : 7;
			uint64_t _ptr : 57;
		};
	};
public:
	MatVar() :
		_raw{ 0 } {}
	explicit MatVar(const MatVar& other) :
		_raw{ other._raw } {}
	explicit MatVar(const void* ptr, TypeBase type = TypeBase::Other) {
		uint64_t p = reinterpret_cast<uint64_t>(ptr);
		assert((p >> 57) == 0 && "MatVar: Pointer exceeds 57 bits!");
		_ptr = p;
		_type = static_cast<uint64_t>(type);
	}
	MatVar& operator=(const MatVar& rhs) {
		if (&rhs == this)
			return *this;

		_raw = rhs._raw;
		return *this;
	}
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, MatVar>>>
	MatVar& operator=(const T& rhs) {
		assert(_ptr != 0 && "MatVar: Assigning to nullptr!");
		assert(_type == TypeBaseFromT<T>() && "MatVar: Wrong input type!");

		*reinterpret_cast<T*>(_ptr) = rhs;
	}

	friend bool operator==(const MatVar& lhs, const MatVar& rhs) {
		return lhs._raw == rhs._raw;
	}
	friend bool operator!=(const MatVar& lhs, const MatVar& rhs) {
		return !(lhs == rhs);
	}
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, MatVar>>>
	friend bool operator==(const MatVar& lhs, const T& rhs) {
		return lhs.get<T>() == rhs;
	}


	void* data() { return reinterpret_cast<void*>(_ptr); }
	void* data() const { return reinterpret_cast<void*>(_ptr); }
	TypeBase type() { return static_cast<TypeBase>(_type); }
	TypeBase type() const { return static_cast<TypeBase>(_type); }

	void setType(TypeBase type) { _type = static_cast<uint64_t>(type); }

	template <typename T>
	T& get() { return *reinterpret_cast<T*>(_ptr); }
	template <typename T>
	T& get() const { return *reinterpret_cast<T*>(_ptr); }
	template <typename T>
	void set(const T& value) { get<T>() = value; }
};




struct alignas(8) MatParam
{
	enum ResourceType : uint8_t
	{
		Unknown = 0,
		Sampler = 0x01,
		CBV = 0x02,
		SRV = 0x04,
		UAV = 0x08
	};

	uint32_t nameIndex;
	uint32_t parentNameIndex;
	std::vector<MatParam> members;
	union
	{
		size_t count;
		size_t size;
	};
	MatParamStage stage;
	uint8_t bindingIndex;
	uint8_t setIndex;
	MatParamArrayType arrayType = MatParamArrayType::None;
	TypeBase type;
	ResourceType resourceType;
	
	uint32_t offset;
	VkDescriptorType descriptorType;

	friend bool operator==(const MatParam& lhs, const MatParam& rhs) {
		return lhs.sharesData(rhs, true);
	}
	friend bool operator!=(const MatParam& lhs, const MatParam& rhs) {
		return !(lhs == rhs);
	}

	bool sharesDataExceptStage(const MatParam& rhs) const {
		return sharesData(rhs, false);
	}

	bool sharesData(const MatParam& rhs, bool includeStage) const {
		if (includeStage && rhs.stage != stage) return false;
		if (rhs.nameIndex != nameIndex) return false;
		if (rhs.parentNameIndex != parentNameIndex) return false;
		if (rhs.bindingIndex != bindingIndex) return false;
		if (rhs.setIndex != setIndex) return false;
		if (rhs.type != type) return false;
		if (rhs.offset != offset) return false;
		if (rhs.count != count) return false;
		if (rhs.members.size() != members.size()) return false;
		for (size_t i = 0; i < rhs.members.size(); ++i) {
			if (!rhs.members[i].sharesData(members[i], includeStage)) return false;
		}
		return true;
	}

	uint32_t varSize() const {
		return static_cast<uint32_t>(SizeOfTypeBase(type));
	}

	uint32_t paddedVarSize() const {
		uint32_t size = SizeOfTypeBase(type);
		for (auto& member : members) {
			size += member.varSize();
		}
		size = (size + 15) & ~15;
		return size;
	}

	std::string& name();
	std::string& name() const;
	std::string& parentName();
	std::string& parentName() const;

	bool isArray() const {
		return count > 0;
	}
};

static MatParam::ResourceType SpvResource_to_ResourceType(int32_t spcType) {

	return (MatParam::ResourceType)static_cast<uint8_t>(spcType);
}


inline static VkShaderStageFlagBits MatParamStageToVkShaderStageFlagBits(MatParamStage stage, MatParam::ResourceType resourceType) {

	VkShaderStageFlagBits matStage = (VkShaderStageFlagBits)0;

	switch (stage) {
		case MatParamStage::Vertex:
			matStage = VK_SHADER_STAGE_VERTEX_BIT; break;
			//return VK_SHADER_STAGE_VERTEX_BIT;
		case MatParamStage::Fragment:
			matStage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
		case MatParamStage::Both:
			matStage = (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); break;

		default: break;// return (VkShaderStageFlagBits)0;
	}

	if (resourceType & MatParam::ResourceType::UAV)
		matStage = (VkShaderStageFlagBits)(matStage | VK_SHADER_STAGE_COMPUTE_BIT);

	return matStage;
}



struct VkBindPair
{
	uint8_t binding;
	uint8_t set;
	bool operator==(const VkBindPair& other) const = default;

};
struct VkBindPairHash
{
	size_t operator()(const VkBindPair& key) const {
		size_t h = 0;
		h ^= std::hash<uint32_t>{}(key.binding)+0x9e3779b9 + (h << 6) + (h >> 2);
		h ^= std::hash<uint32_t>{}(key.set) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}
};

enum class MatBufferType : uint8_t
{
	Uniform,
	SSBO,
	PushConstant
};

class MaterialBuffer
{
private:
	uint8_t* m_memory = nullptr;
	uint32_t m_size;
	uint32_t m_capacity;
	uint32_t m_entrySize;
	VkBindPair m_bind;
	MatBufferType m_bufferType;
	uint8_t m_bindless = 0;
	std::vector<MatParam> m_structure;
	std::unordered_map<std::string, size_t> m_nameIndexMap;
	std::unordered_map<size_t, std::string> m_indexNameMap;
	uint32_t m_nameIndex;
	MatParamStage m_stage;

	void _resize(uint32_t newSize) {
		if (newSize <= m_size)
			return;

		auto temp = new uint8_t[newSize * m_entrySize];
		if (m_memory) {
			std::memcpy(temp, m_memory, m_size * m_entrySize);
			delete[] m_memory;
		}
		m_memory = temp;
		m_capacity = newSize;
	}

public:
	~MaterialBuffer() {
		delete[] m_memory;
	}
	MaterialBuffer() = delete;
	MaterialBuffer(const MatParam& structure);
	MaterialBuffer(const MaterialBuffer& other) {
		m_size = other.m_size;
		m_entrySize = other.m_entrySize;
		if (other.m_capacity > 0) {
			_resize(other.m_capacity);
#pragma warning(push)
#pragma warning(disable : 6387)
			std::memcpy(m_memory, other.m_memory, other.m_size * other.m_entrySize);
#pragma warning(pop)
		} else {
			m_memory = nullptr;
			m_capacity = 0;
		}
		m_bind = other.m_bind;
		m_bufferType = other.m_bufferType;
		m_bindless = other.m_bindless;
		m_structure = other.m_structure;
		m_nameIndexMap = other.m_nameIndexMap;
		m_indexNameMap = other.m_indexNameMap;
		m_stage = other.m_stage;
		m_nameIndex = other.m_nameIndex;
	}
	MaterialBuffer(MaterialBuffer&& other) noexcept {
		m_memory = other.m_memory;
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_entrySize = other.m_entrySize;
		m_bind = other.m_bind;
		m_bufferType = other.m_bufferType;
		m_bindless = other.m_bindless;
		m_structure = other.m_structure;
		m_nameIndexMap = other.m_nameIndexMap;
		m_indexNameMap = other.m_indexNameMap;
		m_stage = other.m_stage;
		m_nameIndex = other.m_nameIndex;

		other.m_memory = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_structure.clear();
		other.m_structure.shrink_to_fit();
		other.m_nameIndexMap.clear();
		other.m_indexNameMap.clear();
	}

	uint32_t push_new() {
		if (m_capacity < (m_size + 1)) {
			_resize(m_size * 2 + 2);
		}
		auto index = m_size++;
		return index;
	}

	uint32_t bufferInfoRange() const {
		return m_entrySize;
	}

	void* const data() const { return m_memory; }

	template <typename T>
	T* getParameter(const std::string& paramName, uint32_t instanceIndex = 0) {
		auto it = m_nameIndexMap.find(paramName);
		if (it != m_nameIndexMap.end()) {
			auto& param = m_structure[it->second];
			uint8_t* ptr = m_memory + instanceIndex * m_entrySize + param.offset;
			return reinterpret_cast<T*>(ptr);
		}
		return nullptr;
	}

	bool setParameter(const std::string& paramName, void* value, uint32_t instanceIndex = 0) {
		auto it = m_nameIndexMap.find(paramName);
		if (it != m_nameIndexMap.end()) {
			auto& param = m_structure[it->second];
			uint8_t* ptr = m_memory + instanceIndex * m_entrySize + param.offset;
			switch (param.type) {
				case TypeBase::UInt32:
					*reinterpret_cast<uint32_t*>(ptr) = *reinterpret_cast<uint32_t*>(value); break;
				case TypeBase::Int32:
					*reinterpret_cast<int32_t*>(ptr) = *reinterpret_cast<int32_t*>(value); break;
				case TypeBase::UInt64:
					*reinterpret_cast<uint64_t*>(ptr) = *reinterpret_cast<uint64_t*>(value); break;
				case TypeBase::Int64:
					*reinterpret_cast<int64_t*>(ptr) = *reinterpret_cast<int64_t*>(value); break;

				case TypeBase::UInt32Vector2:
					*reinterpret_cast<glm::u32vec2*>(ptr) = *reinterpret_cast<glm::u32vec2*>(value); break;
				case TypeBase::UInt32Vector3:
					*reinterpret_cast<glm::u32vec3*>(ptr) = *reinterpret_cast<glm::u32vec3*>(value); break;
				case TypeBase::UInt32Vector4:
					*reinterpret_cast<glm::u32vec4*>(ptr) = *reinterpret_cast<glm::u32vec4*>(value); break;

				case TypeBase::Int32Vector2:
					*reinterpret_cast<glm::i32vec2*>(ptr) = *reinterpret_cast<glm::i32vec2*>(value); break;
				case TypeBase::Int32Vector3:
					*reinterpret_cast<glm::i32vec3*>(ptr) = *reinterpret_cast<glm::i32vec3*>(value); break;
				case TypeBase::Int32Vector4:
					*reinterpret_cast<glm::i32vec4*>(ptr) = *reinterpret_cast<glm::i32vec4*>(value); break;

				case TypeBase::UInt64Vector2:
					*reinterpret_cast<glm::u64vec2*>(ptr) = *reinterpret_cast<glm::u64vec2*>(value); break;
				case TypeBase::UInt64Vector3:
					*reinterpret_cast<glm::u64vec3*>(ptr) = *reinterpret_cast<glm::u64vec3*>(value); break;
				case TypeBase::UInt64Vector4:
					*reinterpret_cast<glm::u64vec4*>(ptr) = *reinterpret_cast<glm::u64vec4*>(value); break;

				case TypeBase::Int64Vector2:
					*reinterpret_cast<glm::i64vec2*>(ptr) = *reinterpret_cast<glm::i64vec2*>(value); break;
				case TypeBase::Int64Vector3:
					*reinterpret_cast<glm::i64vec3*>(ptr) = *reinterpret_cast<glm::i64vec3*>(value); break;
				case TypeBase::Int64Vector4:
					*reinterpret_cast<glm::i64vec4*>(ptr) = *reinterpret_cast<glm::i64vec4*>(value); break;

				case TypeBase::Float:
					*reinterpret_cast<float*>(ptr) = *reinterpret_cast<float*>(value); break;
				case TypeBase::FloatVector2:
					*reinterpret_cast<glm::vec2*>(ptr) = *reinterpret_cast<glm::vec2*>(value); break;
				case TypeBase::FloatVector3:
					*reinterpret_cast<glm::vec3*>(ptr) = *reinterpret_cast<glm::vec3*>(value); break;
				case TypeBase::FloatVector4:
					*reinterpret_cast<glm::vec4*>(ptr) = *reinterpret_cast<glm::vec4*>(value); break;

				case TypeBase::Double:
					*reinterpret_cast<double*>(ptr) = *reinterpret_cast<double*>(value); break;
				case TypeBase::DoubleVector2:
					*reinterpret_cast<glm::f64vec2*>(ptr) = *reinterpret_cast<glm::f64vec2*>(value); break;
				case TypeBase::DoubleVector3:
					*reinterpret_cast<glm::f64vec3*>(ptr) = *reinterpret_cast<glm::f64vec3*>(value); break;
				case TypeBase::DoubleVector4:
					*reinterpret_cast<glm::f64vec4*>(ptr) = *reinterpret_cast<glm::f64vec4*>(value); break;

				case TypeBase::FloatMatrix3x3:
					*reinterpret_cast<glm::mat3x3*>(ptr) = *reinterpret_cast<glm::mat3x3*>(value); break;
				case TypeBase::FloatMatrix4x4:
					*reinterpret_cast<glm::mat4x4*>(ptr) = *reinterpret_cast<glm::mat4x4*>(value); break;
				case TypeBase::DoubleMatrix3x3:
					*reinterpret_cast<glm::dmat3x3*>(ptr) = *reinterpret_cast<glm::dmat3x3*>(value); break;
				case TypeBase::DoubleMatrix4x4:
					*reinterpret_cast<glm::dmat4x4*>(ptr) = *reinterpret_cast<glm::dmat4x4*>(value); break;

				default:
					LOGLINE(LogType::Error, LogMod::Memory,
							std::format("MaterialBuffer: unsupported TypeBase '{}' in setParameter.", TypeBaseToString(param.type)));
					return false;
			}
			return true;
		}
		LOGLINE(LogType::Warning, LogMod::Memory,
				std::format("MaterialBuffer: parameter '{}' not found. Set ignored.", paramName));
		return false;
	}
};



//if constexpr (std::is_same_v<T, uint32_t>) return TypeBase::UInt32;
//if constexpr (std::is_same_v<T, int32_t>) return TypeBase::Int32;
//if constexpr (std::is_same_v<T, uint64_t>) return TypeBase::UInt64;
//if constexpr (std::is_same_v<T, int64_t>) return TypeBase::Int64;
//if constexpr (std::is_same_v<T, glm::u32vec2>) return TypeBase::UInt32Vector2;
//if constexpr (std::is_same_v<T, glm::u32vec3>) return TypeBase::UInt32Vector3;
//if constexpr (std::is_same_v<T, glm::u32vec4>) return TypeBase::UInt32Vector4;
//if constexpr (std::is_same_v<T, glm::i32vec2>) return TypeBase::Int32Vector2;
//if constexpr (std::is_same_v<T, glm::i32vec3>) return TypeBase::Int32Vector3;
//if constexpr (std::is_same_v<T, glm::i32vec4>) return TypeBase::Int32Vector4;
//if constexpr (std::is_same_v<T, glm::u64vec2>) return TypeBase::UInt64Vector2;
//if constexpr (std::is_same_v<T, glm::u64vec3>) return TypeBase::UInt64Vector3;
//if constexpr (std::is_same_v<T, glm::u64vec4>) return TypeBase::UInt64Vector4;
//if constexpr (std::is_same_v<T, glm::i64vec2>) return TypeBase::Int64Vector2;
//if constexpr (std::is_same_v<T, glm::i64vec3>) return TypeBase::Int64Vector3;
//if constexpr (std::is_same_v<T, glm::i64vec4>) return TypeBase::Int64Vector4;
//if constexpr (std::is_same_v<T, float>) return TypeBase::Float;
//if constexpr (std::is_same_v<T, glm::vec2>) return TypeBase::FloatVector2;
//if constexpr (std::is_same_v<T, glm::vec3>) return TypeBase::FloatVector3;
//if constexpr (std::is_same_v<T, glm::vec4>) return TypeBase::FloatVector4;
//if constexpr (std::is_same_v<T, double>) return TypeBase::Double;
//if constexpr (std::is_same_v<T, glm::f64vec2>) return TypeBase::DoubleVector2;
//if constexpr (std::is_same_v<T, glm::f64vec3>) return TypeBase::DoubleVector3;
//if constexpr (std::is_same_v<T, glm::f64vec4>) return TypeBase::DoubleVector4;
//if constexpr (std::is_same_v<T, std::string>) return TypeBase::String;
//if constexpr (std::is_same_v<T, glm::mat3x3>) return TypeBase::FloatMatrix3x3;
//if constexpr (std::is_same_v<T, glm::mat4x4>) return TypeBase::FloatMatrix4x4;
//if constexpr (std::is_same_v<T, glm::dmat3x3>) return TypeBase::DoubleMatrix3x3;
//if constexpr (std::is_same_v<T, glm::dmat4x4>) return TypeBase::DoubleMatrix4x4;
//if constexpr (std::is_same_v<T, Texture2D>) return TypeBase::Texture2D;


























//struct MatParamType
//{
//	enum class Base : uint8_t
//	{
//		None = 0,
//		Invalid,
//		Struct,
//		CBuffer,
//		VertexInput,
//		VertexOutput,
//		Bool,
//		UInt32,
//		Int32,
//		UInt64,
//		Int64,
//		IntVector,
//		IntMatrix,
//		Float,
//		FloatVector,
//		FloatMatrix,
//		Double,
//		DoubleVector,
//		DoubleMatrix,
//		String,
//		Texture,
//		RuntimeArray,
//		Sampler,
//		Other
//	};
//	MatParamType() = default;
//	MatParamType(Base base) : base{base} {}
//	MatParamType(const json& j) { setFromJson(j); }
//
//	Base base = Base::None;
//	uint8_t rows = 0;
//	uint8_t cols = 0;
//
//	void setFromJson(const json& j) {
//		base = fromString(j.at("base").get<std::string>());
//		rows = j.at("rows").get<std::uint8_t>();
//		cols = j.at("cols").get<std::uint8_t>();
//	}
//
//	json toJson() {
//		json j;
//		j["base"] = toString(base);
//		j["rows"] = rows;
//		j["cols"] = cols;
//
//		return j;
//	}
//
//	inline static constexpr const char* toString(MatParamType::Base base) {
//		switch (base) {
//			case MatParamType::Base::None:			return "None";
//			case MatParamType::Base::Invalid:		return "Invalid";
//			case MatParamType::Base::CBuffer:		return "CBuffer";
//			case MatParamType::Base::Struct:		return "Struct";
//			case MatParamType::Base::VertexInput:	return "VertexInput";
//			case MatParamType::Base::VertexOutput:	return "VertexOutput";
//			case MatParamType::Base::Bool:			return "Bool";
//			case MatParamType::Base::UInt32:		return "UInt32";
//			case MatParamType::Base::Int32:			return "Int32";
//			case MatParamType::Base::UInt64:		return "UInt64";
//			case MatParamType::Base::Int64:			return "Int64";
//			case MatParamType::Base::IntVector:		return "IntVector";
//			case MatParamType::Base::IntMatrix:		return "Intmatrix";
//			case MatParamType::Base::Float:			return "Float";
//			case MatParamType::Base::FloatVector:	return "FloatVector";
//			case MatParamType::Base::FloatMatrix:	return "FloatMatrix";
//			case MatParamType::Base::Double:		return "Double";
//			case MatParamType::Base::DoubleVector:	return "DoubleVector";
//			case MatParamType::Base::DoubleMatrix:	return "DoubleMatrix";
//			case MatParamType::Base::String:		return "String";
//			case MatParamType::Base::Texture:		return "Texture";
//			case MatParamType::Base::Other:			return "Other";
//			case MatParamType::Base::RuntimeArray:	return "RuntimeArray";
//			case MatParamType::Base::Sampler:		return "Sampler";
//			default:								return "Invalid";
//		}
//	}
//
//	inline constexpr MatParamType::Base fromString(const std::string& s) {
//		if (s == "None")			return MatParamType::Base::None;
//		if (s == "Invalid")			return MatParamType::Base::Invalid;
//		if (s == "Struct")			return MatParamType::Base::Struct;
//		if (s == "CBuffer")			return MatParamType::Base::CBuffer;
//		if (s == "VertexInput")		return MatParamType::Base::VertexInput;
//		if (s == "VertexOutput")	return MatParamType::Base::VertexOutput;
//		if (s == "Bool")			return MatParamType::Base::Bool;
//		if (s == "UInt32")			return MatParamType::Base::UInt32;
//		if (s == "Int32")			return MatParamType::Base::Int32;
//		if (s == "UInt64")			return MatParamType::Base::UInt64;
//		if (s == "Int64")			return MatParamType::Base::Int64;
//		if (s == "IntVector")		return MatParamType::Base::IntVector;
//		if (s == "FloatVector")		return MatParamType::Base::FloatVector;
//		if (s == "FloatMatrix")		return MatParamType::Base::FloatMatrix;
//		if (s == "Float")			return MatParamType::Base::Float;
//		if (s == "Double")			return MatParamType::Base::Double;
//		if (s == "DoubleVector")	return MatParamType::Base::DoubleVector;
//		if (s == "DoubleMatrix")	return MatParamType::Base::DoubleMatrix;
//		if (s == "String")			return MatParamType::Base::String;
//		if (s == "Texture")			return MatParamType::Base::Texture;
//		if (s == "Other")			return MatParamType::Base::Other;
//		if (s == "RuntimeArray")	return MatParamType::Base::RuntimeArray;
//		if (s == "Sampler")			return MatParamType::Base::Sampler;
//		throw std::invalid_argument("Invalid MatParamType::Base: " + s);
//	}
//
//	//inline constexpr MatParamType::Base deduceMatParamType(VkDescriptorType type) {
//	//	switch (type) {
//	//		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
//	//		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
//	//		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
//	//			return MatParamType::Base::Texture;
//
//	//		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
//	//		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
//	//		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
//	//		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
//	//			return MatParamType::Base::Other;
//
//	//		default:
//	//			return MatParamType::Base::Other;
//	//	}
//	//}
//};
//
//struct MatParam
//{
//	struct alignas (4) MatParamState
//	{
//		MatParamType type;
//		union
//		{
//			uint32_t value;
//			struct
//			{
//				uint32_t count : 30;
//				uint32_t stage : 2;
//			};
//		};
//
//	};
//	union alignas (8) MatParamValue
//	{
//		uint32_t asUint32;
//		int32_t asInt32;
//		uint64_t asUint64;
//		int64_t asInt64;
//		float asFloat;
//		double asDouble;
//		bool asBool;
//		void* asPtr;
//	};
//	MatParamValue value;
//	MatParamState state;
//	std::string parentName;
//	std::string name;
//	std::map<std::string, MatParam> members;
//
//	template <typename T>
//	T& getValueRef() {
//		if (state.count > 1) throw std::exception("MATERIAL: Trying to get single value from array binding.");
//		if constexpr (state.type.base == MatParamType::Base::Bool)		return value.asBool;
//		if constexpr (state.type.base == MatParamType::Base::Float)		return value.asFloat;
//		if constexpr (state.type.base == MatParamType::Base::Double)	return value.asDouble;
//		if constexpr (state.type.base == MatParamType::Base::UInt32)	return value.asUint32;
//		if constexpr (state.type.base == MatParamType::Base::Int32)		return value.asInt32;
//		if constexpr (state.type.base == MatParamType::Base::UInt64)	return value.asUint64;
//		if constexpr (state.type.base == MatParamType::Base::Int64)		return value.asInt64;
//		if constexpr (state.type.base == MatParamType::Base::String)	return *static_cast<std::string*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Texture)	return *static_cast<Texture2D*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Other)		return *static_cast<T*>(value.asPtr);
//	}
//
//	template <typename T>
//	T* getValuePtr(uint32_t& countOut) {
//		countOut = state.count;
//		if constexpr (state.type.base == MatParamType::Base::Bool)		return static_cast<bool*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Float)		return static_cast<float*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Double)	return static_cast<double*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::UInt32)	return static_cast<uint32_t*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Int32)		return static_cast<int32_t*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::UInt64)	return static_cast<uint64_t*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Int64)		return static_cast<int64_t*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::String)	return static_cast<std::string*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Texture)	return static_cast<Texture2D*>(value.asPtr);
//		if constexpr (state.type.base == MatParamType::Base::Other)		return static_cast<T*>(value.asPtr);
//
//		return nullptr;
//	}
//
//
//#ifdef USE_VULKAN
//	uint32_t bindingIndex;
//	VkDescriptorType descriptorType;
//	uint32_t offset;
//
//	json toJson() {
//		json j;
//		j["type"] = state.type.toJson();
//		j["count"] = static_cast<uint32_t>(state.count);
//		j["stage"] = static_cast<uint32_t>(state.stage);
//		j["bindingIndex"] = bindingIndex;
//		j["descriptorType"] = static_cast<uint32_t>(descriptorType);
//		j["offset"] = offset;
//
//		if (state.count > 1) {
//			// Array case
//			switch (state.type.base) {
//				case MatParamType::Base::Bool:
//					j["value"] = std::vector<bool>((bool*)value.asPtr, (bool*)value.asPtr + state.count); break;
//
//				case MatParamType::Base::Float:
//				{
//					if (state.type.cols = 1) {
//						switch (state.type.rows) {
//							case 1: j["value"] = std::vector<float>((float*)value.asPtr, (float*)value.asPtr + state.count); break;
//							case 2: j["value"] = std::vector<std::array<float, 2>>((std::array<float, 2>*)value.asPtr, (std::array<float, 2>*)value.asPtr + state.count); break;
//							case 3: j["value"] = std::vector<std::array<float, 3>>((std::array<float, 3>*)value.asPtr, (std::array<float, 3>*)value.asPtr + state.count); break;
//							case 4: j["value"] = std::vector<std::array<float, 4>>((std::array<float, 4>*)value.asPtr, (std::array<float, 4>*)value.asPtr + state.count); break;
//						}
//					} else if (state.type.cols > 1) {
//
//					}
//					
//				} break;
//
//
//				case MatParamType::Base::Double:
//					j["value"] = std::vector<double>((double*)value.asPtr, (double*)value.asPtr + state.count); break;
//				case MatParamType::Base::UInt32:
//					j["value"] = std::vector<uint32_t>((uint32_t*)value.asPtr, (uint32_t*)value.asPtr + state.count); break;
//				case MatParamType::Base::Int32:
//					j["value"] = std::vector<int32_t>((int32_t*)value.asPtr, (int32_t*)value.asPtr + state.count); break;
//				case MatParamType::Base::UInt64:
//					j["value"] = std::vector<uint64_t>((uint64_t*)value.asPtr, (uint64_t*)value.asPtr + state.count); break;
//				case MatParamType::Base::Int64:
//					j["value"] = std::vector<int64_t>((int64_t*)value.asPtr, (int64_t*)value.asPtr + state.count); break;
//				case MatParamType::Base::String:
//					j["value"] = std::vector<std::string>((std::string*)value.asPtr, (std::string*)value.asPtr + state.count); break;
//				case MatParamType::Base::Other:
//					throw std::runtime_error("Serialization for 'Other' array not implemented");
//				case MatParamType::Base::Texture:
//					throw std::runtime_error("Texture arrays not supported in JSON serialization");
//				default:
//					j["value"] = nullptr;
//			}
//		} else {
//			// Scalar case
//			switch (state.type.base) {
//				case MatParamType::Base::Bool:   j["value"] = value.asBool; break;
//				case MatParamType::Base::Float:  j["value"] = value.asFloat; break;
//				case MatParamType::Base::Double: j["value"] = value.asDouble; break;
//				case MatParamType::Base::UInt32: j["value"] = value.asUint32; break;
//				case MatParamType::Base::Int32:  j["value"] = value.asInt32; break;
//				case MatParamType::Base::UInt64: j["value"] = value.asUint64; break;
//				case MatParamType::Base::Int64:  j["value"] = value.asInt64; break;
//				case MatParamType::Base::String: j["value"] = *static_cast<std::string*>(value.asPtr); break;
//				case MatParamType::Base::Texture: j["value"] = "TexturePointer"; break; // placeholder
//				case MatParamType::Base::Other:   j["value"] = "OtherPointer"; break;   // placeholder
//				default: j["value"] = nullptr;
//			}
//		}
//		return j;
//	}
//
//	void fromJson(const json& j) {
//		state.type = MatParamType{ j };
//		state.stage = j.at("stage").get<uint32_t>();
//		state.count = j.at("count").get<uint32_t>();
//		bindingIndex = j.at("bindingIndex").get<uint32_t>();
//		descriptorType = static_cast<VkDescriptorType>(j.at("descriptorType").get<uint32_t>());
//		offset = j.at("offset").get<uint32_t>();
//
//		if (state.count > 1) {
//			switch (state.type.base) {
//				case MatParamType::Base::Bool:
//				{
//					auto vec = j.at("value").get<std::vector<bool>>();
//					bool* arr = new bool[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::Float:
//				{
//					auto vec = j.at("value").get<std::vector<float>>();
//					float* arr = new float[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::Double:
//				{
//					auto vec = j.at("value").get<std::vector<double>>();
//					double* arr = new double[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::UInt32:
//				{
//					auto vec = j.at("value").get<std::vector<uint32_t>>();
//					uint32_t* arr = new uint32_t[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::Int32:
//				{
//					auto vec = j.at("value").get<std::vector<int32_t>>();
//					int32_t* arr = new int32_t[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::UInt64:
//				{
//					auto vec = j.at("value").get<std::vector<uint64_t>>();
//					uint64_t* arr = new uint64_t[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::Int64:
//				{
//					auto vec = j.at("value").get<std::vector<int64_t>>();
//					int64_t* arr = new int64_t[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::String:
//				{
//					auto vec = j.at("value").get<std::vector<std::string>>();
//					std::string* arr = new std::string[vec.size()];
//					std::copy(vec.begin(), vec.end(), arr);
//					value.asPtr = arr;
//					break;
//				}
//				case MatParamType::Base::Other:
//				case MatParamType::Base::Texture:
//					throw std::runtime_error("Deserialization for Other/Texture array types not supported");
//				default:
//					value.asPtr = nullptr;
//			}
//		} else {
//			// scalar
//			switch (state.type.base) {
//				case MatParamType::Base::Bool:   value.asBool = j.at("value").get<bool>(); break;
//				case MatParamType::Base::Float:  value.asFloat = j.at("value").get<float>(); break;
//				case MatParamType::Base::Double: value.asDouble = j.at("value").get<double>(); break;
//				case MatParamType::Base::UInt32: value.asUint32 = j.at("value").get<uint32_t>(); break;
//				case MatParamType::Base::Int32:  value.asInt32 = j.at("value").get<int32_t>(); break;
//				case MatParamType::Base::UInt64: value.asUint64 = j.at("value").get<uint64_t>(); break;
//				case MatParamType::Base::Int64:  value.asInt64 = j.at("value").get<int64_t>(); break;
//				case MatParamType::Base::String: value.asPtr = new std::string(j.at("value").get<std::string>()); break;
//				case MatParamType::Base::Texture:
//				case MatParamType::Base::Other:
//					throw std::runtime_error("Deserialization for Other/Texture pointer types not supported");
//				default:
//					value.asPtr = nullptr;
//			}
//		}
//	}
//
//#endif



//};


#endif