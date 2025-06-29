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

using json = nlohmann::json;

enum class MatParamStage : uint8_t
{
	Vertex,
	Fragment,
	Both
};


struct MatParamType
{
	enum class Base : uint8_t
	{
		None = 0,
		Invalid,
		Struct,
		CBuffer,
		VertexInput,
		VertexOutput,
		Bool,
		UInt32,
		Int32,
		UInt64,
		Int64,
		IntVector,
		IntMatrix,
		Float,
		FloatVector,
		FloatMatrix,
		Double,
		DoubleVector,
		DoubleMatrix,
		String,
		Texture,
		RuntimeArray,
		Sampler,
		Other
	};
	MatParamType() = default;
	MatParamType(Base base) : base{base} {}
	MatParamType(const json& j) { setFromJson(j); }

	Base base = Base::None;
	uint8_t rows = 0;
	uint8_t cols = 0;

	void setFromJson(const json& j) {
		base = fromString(j.at("base").get<std::string>());
		rows = j.at("rows").get<std::uint8_t>();
		cols = j.at("cols").get<std::uint8_t>();
	}

	json toJson() {
		json j;
		j["base"] = toString(base);
		j["rows"] = rows;
		j["cols"] = cols;

		return j;
	}

	inline static constexpr const char* toString(MatParamType::Base base) {
		switch (base) {
			case MatParamType::Base::None:			return "None";
			case MatParamType::Base::Invalid:		return "Invalid";
			case MatParamType::Base::CBuffer:		return "CBuffer";
			case MatParamType::Base::Struct:		return "Struct";
			case MatParamType::Base::VertexInput:	return "VertexInput";
			case MatParamType::Base::VertexOutput:	return "VertexOutput";
			case MatParamType::Base::Bool:			return "Bool";
			case MatParamType::Base::UInt32:		return "UInt32";
			case MatParamType::Base::Int32:			return "Int32";
			case MatParamType::Base::UInt64:		return "UInt64";
			case MatParamType::Base::Int64:			return "Int64";
			case MatParamType::Base::IntVector:		return "IntVector";
			case MatParamType::Base::IntMatrix:		return "Intmatrix";
			case MatParamType::Base::Float:			return "Float";
			case MatParamType::Base::FloatVector:	return "FloatVector";
			case MatParamType::Base::FloatMatrix:	return "FloatMatrix";
			case MatParamType::Base::Double:		return "Double";
			case MatParamType::Base::DoubleVector:	return "DoubleVector";
			case MatParamType::Base::DoubleMatrix:	return "DoubleMatrix";
			case MatParamType::Base::String:		return "String";
			case MatParamType::Base::Texture:		return "Texture";
			case MatParamType::Base::Other:			return "Other";
			case MatParamType::Base::RuntimeArray:	return "RuntimeArray";
			case MatParamType::Base::Sampler:		return "Sampler";
			default:								return "Invalid";
		}
	}

	inline constexpr MatParamType::Base fromString(const std::string& s) {
		if (s == "None")			return MatParamType::Base::None;
		if (s == "Invalid")			return MatParamType::Base::Invalid;
		if (s == "Struct")			return MatParamType::Base::Struct;
		if (s == "CBuffer")			return MatParamType::Base::CBuffer;
		if (s == "VertexInput")		return MatParamType::Base::VertexInput;
		if (s == "VertexOutput")	return MatParamType::Base::VertexOutput;
		if (s == "Bool")			return MatParamType::Base::Bool;
		if (s == "UInt32")			return MatParamType::Base::UInt32;
		if (s == "Int32")			return MatParamType::Base::Int32;
		if (s == "UInt64")			return MatParamType::Base::UInt64;
		if (s == "Int64")			return MatParamType::Base::Int64;
		if (s == "IntVector")		return MatParamType::Base::IntVector;
		if (s == "FloatVector")		return MatParamType::Base::FloatVector;
		if (s == "FloatMatrix")		return MatParamType::Base::FloatMatrix;
		if (s == "Float")			return MatParamType::Base::Float;
		if (s == "Double")			return MatParamType::Base::Double;
		if (s == "DoubleVector")	return MatParamType::Base::DoubleVector;
		if (s == "DoubleMatrix")	return MatParamType::Base::DoubleMatrix;
		if (s == "String")			return MatParamType::Base::String;
		if (s == "Texture")			return MatParamType::Base::Texture;
		if (s == "Other")			return MatParamType::Base::Other;
		if (s == "RuntimeArray")	return MatParamType::Base::RuntimeArray;
		if (s == "Sampler")			return MatParamType::Base::Sampler;
		throw std::invalid_argument("Invalid MatParamType::Base: " + s);
	}

	//inline constexpr MatParamType::Base deduceMatParamType(VkDescriptorType type) {
	//	switch (type) {
	//		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
	//		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	//		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	//			return MatParamType::Base::Texture;

	//		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	//		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
	//		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	//		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
	//			return MatParamType::Base::Other;

	//		default:
	//			return MatParamType::Base::Other;
	//	}
	//}
};

struct MatParam
{
	struct alignas (4) MatParamState
	{
		MatParamType type;
		union
		{
			uint32_t value;
			struct
			{
				uint32_t count : 30;
				uint32_t stage : 2;
			};
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
	std::string parentName;
	std::string name;
	std::map<std::string, MatParam> members;

	template <typename T>
	T& getValueRef() {
		if (state.count > 1) throw std::exception("MATERIAL: Trying to get single value from array binding.");
		if constexpr (state.type.base == MatParamType::Base::Bool)		return value.asBool;
		if constexpr (state.type.base == MatParamType::Base::Float)		return value.asFloat;
		if constexpr (state.type.base == MatParamType::Base::Double)	return value.asDouble;
		if constexpr (state.type.base == MatParamType::Base::UInt32)	return value.asUint32;
		if constexpr (state.type.base == MatParamType::Base::Int32)		return value.asInt32;
		if constexpr (state.type.base == MatParamType::Base::UInt64)	return value.asUint64;
		if constexpr (state.type.base == MatParamType::Base::Int64)		return value.asInt64;
		if constexpr (state.type.base == MatParamType::Base::String)	return *static_cast<std::string*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Texture)	return *static_cast<Texture2D*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Other)		return *static_cast<T*>(value.asPtr);
	}

	template <typename T>
	T* getValuePtr(uint32_t& countOut) {
		countOut = state.count;
		if constexpr (state.type.base == MatParamType::Base::Bool)		return static_cast<bool*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Float)		return static_cast<float*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Double)	return static_cast<double*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::UInt32)	return static_cast<uint32_t*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Int32)		return static_cast<int32_t*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::UInt64)	return static_cast<uint64_t*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Int64)		return static_cast<int64_t*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::String)	return static_cast<std::string*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Texture)	return static_cast<Texture2D*>(value.asPtr);
		if constexpr (state.type.base == MatParamType::Base::Other)		return static_cast<T*>(value.asPtr);

		return nullptr;
	}


#ifdef USE_VULKAN
	uint32_t bindingIndex;
	VkDescriptorType descriptorType;
	uint32_t offset;

	json toJson() {
		json j;
		j["type"] = state.type.toJson();
		j["count"] = static_cast<uint32_t>(state.count);
		j["stage"] = static_cast<uint32_t>(state.stage);
		j["bindingIndex"] = bindingIndex;
		j["descriptorType"] = static_cast<uint32_t>(descriptorType);
		j["offset"] = offset;

		if (state.count > 1) {
			// Array case
			switch (state.type.base) {
				case MatParamType::Base::Bool:
					j["value"] = std::vector<bool>((bool*)value.asPtr, (bool*)value.asPtr + state.count); break;

				case MatParamType::Base::Float:
				{
					if (state.type.cols = 1) {
						switch (state.type.rows) {
							case 1: j["value"] = std::vector<float>((float*)value.asPtr, (float*)value.asPtr + state.count); break;
							case 2: j["value"] = std::vector<std::array<float, 2>>((std::array<float, 2>*)value.asPtr, (std::array<float, 2>*)value.asPtr + state.count); break;
							case 3: j["value"] = std::vector<std::array<float, 3>>((std::array<float, 3>*)value.asPtr, (std::array<float, 3>*)value.asPtr + state.count); break;
							case 4: j["value"] = std::vector<std::array<float, 4>>((std::array<float, 4>*)value.asPtr, (std::array<float, 4>*)value.asPtr + state.count); break;
						}
					} else if (state.type.cols > 1) {

					}
					
				} break;


				case MatParamType::Base::Double:
					j["value"] = std::vector<double>((double*)value.asPtr, (double*)value.asPtr + state.count); break;
				case MatParamType::Base::UInt32:
					j["value"] = std::vector<uint32_t>((uint32_t*)value.asPtr, (uint32_t*)value.asPtr + state.count); break;
				case MatParamType::Base::Int32:
					j["value"] = std::vector<int32_t>((int32_t*)value.asPtr, (int32_t*)value.asPtr + state.count); break;
				case MatParamType::Base::UInt64:
					j["value"] = std::vector<uint64_t>((uint64_t*)value.asPtr, (uint64_t*)value.asPtr + state.count); break;
				case MatParamType::Base::Int64:
					j["value"] = std::vector<int64_t>((int64_t*)value.asPtr, (int64_t*)value.asPtr + state.count); break;
				case MatParamType::Base::String:
					j["value"] = std::vector<std::string>((std::string*)value.asPtr, (std::string*)value.asPtr + state.count); break;
				case MatParamType::Base::Other:
					throw std::runtime_error("Serialization for 'Other' array not implemented");
				case MatParamType::Base::Texture:
					throw std::runtime_error("Texture arrays not supported in JSON serialization");
				default:
					j["value"] = nullptr;
			}
		} else {
			// Scalar case
			switch (state.type.base) {
				case MatParamType::Base::Bool:   j["value"] = value.asBool; break;
				case MatParamType::Base::Float:  j["value"] = value.asFloat; break;
				case MatParamType::Base::Double: j["value"] = value.asDouble; break;
				case MatParamType::Base::UInt32: j["value"] = value.asUint32; break;
				case MatParamType::Base::Int32:  j["value"] = value.asInt32; break;
				case MatParamType::Base::UInt64: j["value"] = value.asUint64; break;
				case MatParamType::Base::Int64:  j["value"] = value.asInt64; break;
				case MatParamType::Base::String: j["value"] = *static_cast<std::string*>(value.asPtr); break;
				case MatParamType::Base::Texture: j["value"] = "TexturePointer"; break; // placeholder
				case MatParamType::Base::Other:   j["value"] = "OtherPointer"; break;   // placeholder
				default: j["value"] = nullptr;
			}
		}
		return j;
	}

	void fromJson(const json& j) {
		state.type = MatParamType{ j };
		state.stage = j.at("stage").get<uint32_t>();
		state.count = j.at("count").get<uint32_t>();
		bindingIndex = j.at("bindingIndex").get<uint32_t>();
		descriptorType = static_cast<VkDescriptorType>(j.at("descriptorType").get<uint32_t>());
		offset = j.at("offset").get<uint32_t>();

		if (state.count > 1) {
			switch (state.type.base) {
				case MatParamType::Base::Bool:
				{
					auto vec = j.at("value").get<std::vector<bool>>();
					bool* arr = new bool[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::Float:
				{
					auto vec = j.at("value").get<std::vector<float>>();
					float* arr = new float[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::Double:
				{
					auto vec = j.at("value").get<std::vector<double>>();
					double* arr = new double[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::UInt32:
				{
					auto vec = j.at("value").get<std::vector<uint32_t>>();
					uint32_t* arr = new uint32_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::Int32:
				{
					auto vec = j.at("value").get<std::vector<int32_t>>();
					int32_t* arr = new int32_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::UInt64:
				{
					auto vec = j.at("value").get<std::vector<uint64_t>>();
					uint64_t* arr = new uint64_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::Int64:
				{
					auto vec = j.at("value").get<std::vector<int64_t>>();
					int64_t* arr = new int64_t[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::String:
				{
					auto vec = j.at("value").get<std::vector<std::string>>();
					std::string* arr = new std::string[vec.size()];
					std::copy(vec.begin(), vec.end(), arr);
					value.asPtr = arr;
					break;
				}
				case MatParamType::Base::Other:
				case MatParamType::Base::Texture:
					throw std::runtime_error("Deserialization for Other/Texture array types not supported");
				default:
					value.asPtr = nullptr;
			}
		} else {
			// scalar
			switch (state.type.base) {
				case MatParamType::Base::Bool:   value.asBool = j.at("value").get<bool>(); break;
				case MatParamType::Base::Float:  value.asFloat = j.at("value").get<float>(); break;
				case MatParamType::Base::Double: value.asDouble = j.at("value").get<double>(); break;
				case MatParamType::Base::UInt32: value.asUint32 = j.at("value").get<uint32_t>(); break;
				case MatParamType::Base::Int32:  value.asInt32 = j.at("value").get<int32_t>(); break;
				case MatParamType::Base::UInt64: value.asUint64 = j.at("value").get<uint64_t>(); break;
				case MatParamType::Base::Int64:  value.asInt64 = j.at("value").get<int64_t>(); break;
				case MatParamType::Base::String: value.asPtr = new std::string(j.at("value").get<std::string>()); break;
				case MatParamType::Base::Texture:
				case MatParamType::Base::Other:
					throw std::runtime_error("Deserialization for Other/Texture pointer types not supported");
				default:
					value.asPtr = nullptr;
			}
		}
	}

#endif


	inline static constexpr const char* matParamStageToString(MatParamStage stage) {
		switch (stage) {
			case MatParamStage::Vertex:   return "Vertex";
			case MatParamStage::Fragment: return "Fragment";
			case MatParamStage::Both:     return "Both";
			default:                      return "Unknown";
		}
	}

	inline constexpr MatParamStage matParamStageFromString(const std::string& s) {
		if (s == "Vertex")   return MatParamStage::Vertex;
		if (s == "Fragment") return MatParamStage::Fragment;
		if (s == "Both")     return MatParamStage::Both;
		throw std::invalid_argument("Invalid MatParamStage: " + s);
	}

};


#endif