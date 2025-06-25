#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#ifdef USE_VULKAN
	#ifndef VULKAN_CORE_H_
		#include <vulkan/vulkan_core.h>
	#endif
#endif

#include "Texture.hpp"

enum class MatParamStage : uint8_t
{
	Vertex,
	Fragment,
	Both
};

enum class MatParamType : uint8_t
{
	None,
	Bool,
	UInt32,
	Int32,
	UInt64,
	Int64,
	Float,
	Double,
	String,
	Texture,
	Other
};

namespace MatParamUtils
{
	inline const char* toString(MatParamStage stage) {
		switch (stage) {
			case MatParamStage::Vertex:   return "Vertex";
			case MatParamStage::Fragment: return "Fragment";
			case MatParamStage::Both:     return "Both";
			default:                      return "Unknown";
		}
	}

	inline MatParamStage matParamStageFromString(const std::string& s) {
		if (s == "Vertex")   return MatParamStage::Vertex;
		if (s == "Fragment") return MatParamStage::Fragment;
		if (s == "Both")     return MatParamStage::Both;
		throw std::invalid_argument("Invalid MatParamStage: " + s);
	}

	inline const char* toString(MatParamType type) {
		switch (type) {
			case MatParamType::None:    return "None";
			case MatParamType::Bool:    return "Bool";
			case MatParamType::UInt32:  return "UInt32";
			case MatParamType::Int32:   return "Int32";
			case MatParamType::UInt64:  return "UInt64";
			case MatParamType::Int64:   return "Int64";
			case MatParamType::Float:   return "Float";
			case MatParamType::Double:  return "Double";
			case MatParamType::String:  return "String";
			case MatParamType::Texture: return "Texture";
			case MatParamType::Other:   return "Other";
			default:                    return "Unknown";
		}
	}

	inline MatParamType matParamTypeFromString(const std::string& s) {
		if (s == "None")    return MatParamType::None;
		if (s == "Bool")    return MatParamType::Bool;
		if (s == "UInt32")  return MatParamType::UInt32;
		if (s == "Int32")   return MatParamType::Int32;
		if (s == "UInt64")  return MatParamType::UInt64;
		if (s == "Int64")   return MatParamType::Int64;
		if (s == "Float")   return MatParamType::Float;
		if (s == "Double")  return MatParamType::Double;
		if (s == "String")  return MatParamType::String;
		if (s == "Texture") return MatParamType::Texture;
		if (s == "Other")   return MatParamType::Other;
		throw std::invalid_argument("Invalid MatParamType: " + s);
	}


#ifdef USE_VULKAN
	inline MatParamType deduceMatParamType(VkDescriptorType type) {
		switch (type) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				return MatParamType::Texture;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				return MatParamType::Other;

			default:
				return MatParamType::Other;
		}
	}
#endif
}

struct MatParam
{
	union alignas (8) MatParamState
	{
		uint32_t value;
		struct
		{
			MatParamType type		: 8;
			MatParamStage stage		: 2;
			uint32_t count			: 22;
		};
	};
	union alignas (8) MatParamValue
	{
		uint32_t asUint32;
		int32_t asInt32;
		uint64_t asUint64;
		int64_t asInt64;
		float asFloat;
		double asDouble;
		bool asBool;
		void* asPtr;
	};
	MatParamValue value;
	MatParamState state;

	template <typename T>
	T& getValueRef() {
		if (state.count > 1) throw std::exception("MATERIAL: Trying to get single value from array binding.");
		if constexpr (state.type == MatParamType::Bool)		return value.asBool;
		if constexpr (state.type == MatParamType::Float)	return value.asFloat;
		if constexpr (state.type == MatParamType::Double)	return value.asDouble;
		if constexpr (state.type == MatParamType::UInt32)	return value.asUint32;
		if constexpr (state.type == MatParamType::Int32)	return value.asInt32;
		if constexpr (state.type == MatParamType::UInt64)	return value.asUint64;
		if constexpr (state.type == MatParamType::Int64)	return value.asInt64;
		if constexpr (state.type == MatParamType::String)	return *static_cast<std::string*>(value.asPtr);
		if constexpr (state.type == MatParamType::Texture)	return *static_cast<Texture2D*>(value.asPtr);
		if constexpr (state.type == MatParamType::Other)	return *static_cast<T*>(value.asPtr);
	}

	template <typename T>
	T* getValuePtr(uint32_t& countOut) {
		countOut = state.count;
		if constexpr (state.type == MatParamType::Bool)		return static_cast<bool*>(value.asPtr);
		if constexpr (state.type == MatParamType::Float)	return static_cast<float*>(value.asPtr);
		if constexpr (state.type == MatParamType::Double)	return static_cast<double*>(value.asPtr);
		if constexpr (state.type == MatParamType::UInt32)	return static_cast<uint32_t*>(value.asPtr);
		if constexpr (state.type == MatParamType::Int32)	return static_cast<int32_t*>(value.asPtr);
		if constexpr (state.type == MatParamType::UInt64)	return static_cast<uint64_t*>(value.asPtr);
		if constexpr (state.type == MatParamType::Int64)	return static_cast<int64_t*>(value.asPtr);
		if constexpr (state.type == MatParamType::String)	return static_cast<std::string*>(value.asPtr);
		if constexpr (state.type == MatParamType::Texture)	return static_cast<Texture2D*>(value.asPtr);
		if constexpr (state.type == MatParamType::Other)	return static_cast<T*>(value.asPtr);

		return nullptr;
	}


#ifdef USE_VULKAN
	uint32_t bindingIndex;
	VkDescriptorType descriptorType;
	uint32_t offset;

	json toJson() const {
		json j;
		j["type"] = MatParamUtils::toString(state.type);
		j["stage"] = MatParamUtils::toString(state.stage);
		j["count"] = state.count;

		if (state.count > 1) {
			// Array case
			switch (state.type) {
				case MatParamType::Bool:
					j["value"] = std::vector<bool>((bool*)value.asPtr, (bool*)value.asPtr + state.count); break;
				case MatParamType::Float:
					j["value"] = std::vector<float>((float*)value.asPtr, (float*)value.asPtr + state.count); break;
				case MatParamType::Double:
					j["value"] = std::vector<double>((double*)value.asPtr, (double*)value.asPtr + state.count); break;
				case MatParamType::UInt32:
					j["value"] = std::vector<uint32_t>((uint32_t*)value.asPtr, (uint32_t*)value.asPtr + state.count); break;
				case MatParamType::Int32:
					j["value"] = std::vector<int32_t>((int32_t*)value.asPtr, (int32_t*)value.asPtr + state.count); break;
				case MatParamType::UInt64:
					j["value"] = std::vector<uint64_t>((uint64_t*)value.asPtr, (uint64_t*)value.asPtr + state.count); break;
				case MatParamType::Int64:
					j["value"] = std::vector<int64_t>((int64_t*)value.asPtr, (int64_t*)value.asPtr + state.count); break;
				case MatParamType::String:
					j["value"] = std::vector<std::string>((std::string*)value.asPtr, (std::string*)value.asPtr + state.count); break;
				case MatParamType::Other:
					throw std::runtime_error("Serialization for 'Other' array not implemented");
				case MatParamType::Texture:
					throw std::runtime_error("Texture arrays not supported in JSON serialization");
				default:
					j["value"] = nullptr;
			}
		} else {
			// Scalar case
			switch (state.type) {
				case MatParamType::Bool:   j["value"] = value.asBool; break;
				case MatParamType::Float:  j["value"] = value.asFloat; break;
				case MatParamType::Double: j["value"] = value.asDouble; break;
				case MatParamType::UInt32: j["value"] = value.asUint32; break;
				case MatParamType::Int32:  j["value"] = value.asInt32; break;
				case MatParamType::UInt64: j["value"] = value.asUint64; break;
				case MatParamType::Int64:  j["value"] = value.asInt64; break;
				case MatParamType::String: j["value"] = *static_cast<std::string*>(value.asPtr); break;
				case MatParamType::Texture: j["value"] = "TexturePointer"; break; // placeholder
				case MatParamType::Other:   j["value"] = "OtherPointer"; break;   // placeholder
				default: j["value"] = nullptr;
			}
		}
		return j;
	}

	void fromJson(const json& j) {
		state.type = MatParamUtils::matParamTypeFromString(j.at("type").get<std::string>());
		state.stage = MatParamUtils::matParamStageFromString(j.at("stage").get<std::string>());
		state.count = j.at("count").get<uint32_t>();

		if (state.count > 1) {
			switch (state.type) {
				case MatParamType::Bool:
				{
					auto vec = j.at("value").get<std::vector<bool>>();
					bool* arr = new bool[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Float:
				{
					auto vec = j.at("value").get<std::vector<float>>();
					float* arr = new float[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Double:
				{
					auto vec = j.at("value").get<std::vector<double>>();
					double* arr = new double[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::UInt32:
				{
					auto vec = j.at("value").get<std::vector<uint32_t>>();
					uint32_t* arr = new uint32_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Int32:
				{
					auto vec = j.at("value").get<std::vector<int32_t>>();
					int32_t* arr = new int32_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::UInt64:
				{
					auto vec = j.at("value").get<std::vector<uint64_t>>();
					uint64_t* arr = new uint64_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Int64:
				{
					auto vec = j.at("value").get<std::vector<int64_t>>();
					int64_t* arr = new int64_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::String:
				{
					auto vec = j.at("value").get<std::vector<std::string>>();
					std::string* arr = new std::string[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Other:
				case MatParamType::Texture:
					throw std::runtime_error("Deserialization for Other/Texture array types not supported");
				default:
					value.asPtr = nullptr;
			}
		} else {
			// scalar
			switch (state.type) {
				case MatParamType::Bool:   value.asBool = j.at("value").get<bool>(); break;
				case MatParamType::Float:  value.asFloat = j.at("value").get<float>(); break;
				case MatParamType::Double: value.asDouble = j.at("value").get<double>(); break;
				case MatParamType::UInt32: value.asUint32 = j.at("value").get<uint32_t>(); break;
				case MatParamType::Int32:  value.asInt32 = j.at("value").get<int32_t>(); break;
				case MatParamType::UInt64: value.asUint64 = j.at("value").get<uint64_t>(); break;
				case MatParamType::Int64:  value.asInt64 = j.at("value").get<int64_t>(); break;
				case MatParamType::String: value.asPtr = new std::string(j.at("value").get<std::string>()); break;
				case MatParamType::Texture:
				case MatParamType::Other:
					throw std::runtime_error("Deserialization for Other/Texture pointer types not supported");
				default:
					value.asPtr = nullptr;
			}
		}
	}

#endif
};


class Shader;
class ShaderManager;
class Material
{
private:
	friend ShaderManager;
	size_t m_arrayIndex;
	void fromJson(const json& j);
public:
	std::vector<MatParam> params;
	std::string vShaderName;
	std::string pShaderName;

	Material() = default;
	Material(const json& j) { fromJson(j); }
	Material(const std::string& name, Shader& vertex, Shader& fragment) { initFromShaders(name, vertex, fragment); }

	std::string& name();

	json toJson();
	

	void initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);
};

#endif