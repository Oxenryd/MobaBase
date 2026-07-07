#ifndef TYPEREFLECTION_HPP
#define TYPEREFLECTION_HPP

#include <cstdint>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <utility>
#include <span>

#include "ArenaAllocator.hpp"
#include <glm/glm.hpp>
#include "Texture.hpp"

enum class TypeBase : uint8_t
{
	None = 0,
	Invalid,
	Struct,
	StructBuffer,
	RWStructBuffer,
	CBuffer,
	VertexInput,
	VertexOutput,
	Bool,
	UInt32,
	Int32,
	UInt64,
	Int64,
	UInt32Vector2,
	UInt32Vector3,
	UInt32Vector4,
	Int32Vector2,
	Int32Vector3,
	Int32Vector4,
	UInt64Vector2,
	UInt64Vector3,
	UInt64Vector4,
	Int64Vector2,
	Int64Vector3,
	Int64Vector4,
	Float,
	FloatVector2,
	FloatVector3,
	FloatVector4,
	FloatMatrix3x3,
	FloatMatrix4x4,
	Double,
	DoubleVector2,
	DoubleVector3,
	DoubleVector4,
	DoubleMatrix3x3,
	DoubleMatrix4x4,
	String,
	Texture2D,
	Texture2DArray,
	RuntimeArray,
	Sampler,
	PushConstStruct,
	PushConst,
	Other,
	UnimplementedMatrix,
	FloatMatrix2x2,
	DoubleMatrix2x2
};

constexpr bool TypeBaseIsAllocatable(const TypeBase type) {

	switch (type) {
		default:
			return true;

		case TypeBase::None:
		case TypeBase::Invalid:
		case TypeBase::VertexInput:
		case TypeBase::VertexOutput:
		case TypeBase::String:
		case TypeBase::Texture2D:
		case TypeBase::RuntimeArray:
		case TypeBase::Sampler:
			return false;
	}
}

constexpr TypeBase TypeBaseFromUint64(uint64_t value) {
	return static_cast<TypeBase>(value);
}

// Detect std::vector
template<typename>
struct is_std_vector : std::false_type {};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

template<typename T>
constexpr bool is_std_vector_v = is_std_vector<std::remove_cv_t<T>>::value;

// Detect std::array
template<typename T>
struct array_size;

template<typename T, std::size_t N>
struct array_size<std::array<T, N>>
{
	static constexpr std::size_t value = N;
};

template<typename>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename T>
constexpr bool is_std_array_v = is_std_array<std::remove_cv_t<T>>::value;

// Combined trait
template<typename T>
constexpr bool is_array_like_v = is_std_vector_v<T> || is_std_array_v<T>;

template<typename T>
constexpr std::size_t array_size_v = array_size<std::remove_cv_t<T>>::value;


using TypeBaseList = std::tuple<
	void,
	void,
	void,
	void,
	void,
	void,
	void,
	void,
	bool,
	uint32_t,
	int32_t,
	uint64_t,
	int64_t,
	glm::u32vec2,
	glm::u32vec3,
	glm::u32vec4,
	glm::i32vec2,
	glm::i32vec3,
	glm::i32vec4,
	glm::u64vec2,
	glm::u64vec3,
	glm::u64vec4,
	glm::i64vec2,
	glm::i64vec3,
	glm::i64vec4,
	float,
	glm::vec2,
	glm::vec3,
	glm::vec4,
	glm::mat3x3,
	glm::mat4x4,
	double,
	glm::f64vec2,
	glm::f64vec3,
	glm::f64vec4,
	glm::dmat3x3,
	glm::dmat4x4,
	std::string,
	Texture2D,
	Texture2D*,
	void,
	void,
	void,
	void,
	void
>;

template<TypeBase E>
using TypeBase_t = std::tuple_element_t<static_cast<size_t>(E), TypeBaseList>;

template<uint64_t E>
using TypeBase_t_uint = std::tuple_element_t<E, TypeBaseList>;

template <typename T>
static constexpr TypeBase TypeBaseFromT() {

	if constexpr (std::is_same_v<T, uint32_t>) return TypeBase::UInt32;
	if constexpr (std::is_same_v<T, int32_t>) return TypeBase::Int32;
	if constexpr (std::is_same_v<T, uint64_t>) return TypeBase::UInt64;
	if constexpr (std::is_same_v<T, int64_t>) return TypeBase::Int64;
	if constexpr (std::is_same_v<T, glm::u32vec2>) return TypeBase::UInt32Vector2;
	if constexpr (std::is_same_v<T, glm::u32vec3>) return TypeBase::UInt32Vector3;
	if constexpr (std::is_same_v<T, glm::u32vec4>) return TypeBase::UInt32Vector4;
	if constexpr (std::is_same_v<T, glm::i32vec2>) return TypeBase::Int32Vector2;
	if constexpr (std::is_same_v<T, glm::i32vec3>) return TypeBase::Int32Vector3;
	if constexpr (std::is_same_v<T, glm::i32vec4>) return TypeBase::Int32Vector4;
	if constexpr (std::is_same_v<T, glm::u64vec2>) return TypeBase::UInt64Vector2;
	if constexpr (std::is_same_v<T, glm::u64vec3>) return TypeBase::UInt64Vector3;
	if constexpr (std::is_same_v<T, glm::u64vec4>) return TypeBase::UInt64Vector4;
	if constexpr (std::is_same_v<T, glm::i64vec2>) return TypeBase::Int64Vector2;
	if constexpr (std::is_same_v<T, glm::i64vec3>) return TypeBase::Int64Vector3;
	if constexpr (std::is_same_v<T, glm::i64vec4>) return TypeBase::Int64Vector4;
	if constexpr (std::is_same_v<T, float>) return TypeBase::Float;
	if constexpr (std::is_same_v<T, glm::vec2>) return TypeBase::FloatVector2;
	if constexpr (std::is_same_v<T, glm::vec3>) return TypeBase::FloatVector3;
	if constexpr (std::is_same_v<T, glm::vec4>) return TypeBase::FloatVector4;
	if constexpr (std::is_same_v<T, double>) return TypeBase::Double;
	if constexpr (std::is_same_v<T, glm::f64vec2>) return TypeBase::DoubleVector2;
	if constexpr (std::is_same_v<T, glm::f64vec3>) return TypeBase::DoubleVector3;
	if constexpr (std::is_same_v<T, glm::f64vec4>) return TypeBase::DoubleVector4;
	if constexpr (std::is_same_v<T, std::string>) return TypeBase::String;
	if constexpr (std::is_same_v<T, glm::mat3x3>) return TypeBase::FloatMatrix3x3;
	if constexpr (std::is_same_v<T, glm::mat4x4>) return TypeBase::FloatMatrix4x4;
	if constexpr (std::is_same_v<T, glm::dmat3x3>) return TypeBase::DoubleMatrix3x3;
	if constexpr (std::is_same_v<T, glm::dmat4x4>) return TypeBase::DoubleMatrix4x4;
	if constexpr (std::is_same_v<T, Texture2D>) return TypeBase::Texture2D;

	return TypeBase::None;
}


template <typename T>
static T ReadValueAs(void* ptr) {
	return *static_cast<T*>(ptr);
}
template <typename T>
static T& ReadRefAs(void* ptr) {
	return *static_cast<T*>(ptr);
}

template <typename T>
static constexpr std::pair<size_t, size_t> allocSizeAlignment() {

	if constexpr (TypeBaseFromT<T>() != TypeBase::Other) {
		return {sizeof(T), alignof(T)};
	}

	return {sizeof(void*), alignof(void*)};
}

static constexpr const char* TypeBaseToString(const TypeBase base) {
	switch (base) {
		case TypeBase::None:			return "None";
		case TypeBase::Invalid:			return "Invalid";
		case TypeBase::CBuffer:			return "CBuffer";
		case TypeBase::Struct:			return "Struct";
		case TypeBase::StructBuffer:	return "StructBuffer";
		case TypeBase::RWStructBuffer:	return "RWStructBuffer";
		case TypeBase::VertexInput:		return "VertexInput";
		case TypeBase::VertexOutput:	return "VertexOutput";
		case TypeBase::Bool:			return "Bool";
		case TypeBase::UInt32:			return "UInt32";
		case TypeBase::Int32:			return "Int32";
		case TypeBase::UInt64:			return "UInt64";
		case TypeBase::Int64:			return "Int64";
		case TypeBase::UInt32Vector2:	return "UInt32Vector2";
		case TypeBase::UInt32Vector3:	return "UInt32Vector3";
		case TypeBase::UInt32Vector4:	return "UInt32Vector4";
		case TypeBase::Int32Vector2:	return "Int32Vector2";
		case TypeBase::Int32Vector3:	return "Int32Vector3";
		case TypeBase::Int32Vector4:	return "Int32Vector4";
		case TypeBase::UInt64Vector2:	return "UInt64Vector2";
		case TypeBase::UInt64Vector3:	return "UInt64Vector3";
		case TypeBase::UInt64Vector4:	return "UInt64Vector4";
		case TypeBase::Int64Vector2:	return "Int64Vector2";
		case TypeBase::Int64Vector3:	return "Int64Vector3";
		case TypeBase::Int64Vector4:	return "Int64Vector4";
		case TypeBase::Float:			return "Float";
		case TypeBase::FloatVector2:	return "FloatVector2";
		case TypeBase::FloatVector3:	return "FloatVector3";
		case TypeBase::FloatVector4:	return "FloatVector4";
		case TypeBase::FloatMatrix3x3:	return "FloatMatrix3x3";
		case TypeBase::FloatMatrix4x4:	return "FloatMatrix4x4";
		case TypeBase::Double:			return "Double";
		case TypeBase::DoubleVector2:	return "DoubleVector2";
		case TypeBase::DoubleVector3:	return "DoubleVector3";
		case TypeBase::DoubleVector4:	return "DoubleVector4";
		case TypeBase::DoubleMatrix3x3:	return "DoubleMatrix3x3";
		case TypeBase::DoubleMatrix4x4:	return "DoubleMatrix4x4";
		case TypeBase::String:			return "String";
		case TypeBase::Texture2D:		return "Texture2D";
		case TypeBase::Texture2DArray:	return "Texture2DArray";
		case TypeBase::Other:			return "Other";
		case TypeBase::RuntimeArray:	return "RuntimeArray";
		case TypeBase::Sampler:			return "Sampler";
		case TypeBase::PushConstStruct:	return "PushConstStruct";
		case TypeBase::PushConst:		return "PushConst";
		default:						return "Invalid";
	}
}

constexpr TypeBase stringToTypeBase(const std::string& s) {
	if (s == "None")			return TypeBase::None;
	if (s == "Invalid")			return TypeBase::Invalid;
	if (s == "Struct")			return TypeBase::Struct;
	if (s == "StructBuffer")	return TypeBase::StructBuffer;
	if (s == "RWStructBuffer")	return TypeBase::StructBuffer;
	if (s == "CBuffer")			return TypeBase::CBuffer;
	if (s == "VertexInput")		return TypeBase::VertexInput;
	if (s == "VertexOutput")	return TypeBase::VertexOutput;
	if (s == "Bool")			return TypeBase::Bool;
	if (s == "UInt32")			return TypeBase::UInt32;
	if (s == "Int32")			return TypeBase::Int32;
	if (s == "UInt64")			return TypeBase::UInt64;
	if (s == "Int64")			return TypeBase::Int64;
	if (s == "UInt32Vector2")	return TypeBase::UInt32Vector2;
	if (s == "UInt32Vector3")	return TypeBase::UInt32Vector3;
	if (s == "UInt32Vector4")	return TypeBase::UInt32Vector4;
	if (s == "Int32Vector2")	return TypeBase::Int32Vector2;
	if (s == "Int32Vector3")	return TypeBase::Int32Vector3;
	if (s == "Int32Vector4")	return TypeBase::Int32Vector4;
	if (s == "UInt64Vector2")	return TypeBase::UInt64Vector2;
	if (s == "UInt64Vector3")	return TypeBase::UInt64Vector3;
	if (s == "UInt64Vector4")	return TypeBase::UInt64Vector4;
	if (s == "Int64Vector2")	return TypeBase::Int64Vector2;
	if (s == "Int64Vector3")	return TypeBase::Int64Vector3;
	if (s == "Int64Vector4")	return TypeBase::Int64Vector4;
	if (s == "FloatVector2")	return TypeBase::FloatVector2;
	if (s == "FloatVector3")	return TypeBase::FloatVector3;
	if (s == "FloatVector4")	return TypeBase::FloatVector4;
	if (s == "FloatMatrix3x3")	return TypeBase::FloatMatrix3x3;
	if (s == "FloatMatrix4x4")	return TypeBase::FloatMatrix4x4;
	if (s == "Float")			return TypeBase::Float;
	if (s == "Double")			return TypeBase::Double;
	if (s == "DoubleVector2")	return TypeBase::DoubleVector2;
	if (s == "DoubleVector3")	return TypeBase::DoubleVector3;
	if (s == "DoubleVector4")	return TypeBase::DoubleVector4;
	if (s == "DoubleMatrix3x3")	return TypeBase::DoubleMatrix3x3;
	if (s == "DoubleMatrix4x4")	return TypeBase::DoubleMatrix4x4;
	if (s == "String")			return TypeBase::String;
	if (s == "Texture2D")		return TypeBase::Texture2D;
	if (s == "Texture2DArray")	return TypeBase::Texture2DArray;
	if (s == "Other")			return TypeBase::Other;
	if (s == "RuntimeArray")	return TypeBase::RuntimeArray;
	if (s == "Sampler")			return TypeBase::Sampler;
	if (s == "PushConst")		return TypeBase::PushConst;
	if (s == "PushConstStruct") return TypeBase::PushConstStruct;
	throw std::invalid_argument("Invalid TypeBase: " + s);
}


static constexpr size_t SizeOfTypeBase(const TypeBase type) {
	switch (type) {
		case TypeBase::None:			return 0;
		case TypeBase::Invalid:			return 0;
		case TypeBase::CBuffer:			return 0;
		case TypeBase::Struct:			return 0;
		case TypeBase::VertexInput:		return 0;
		case TypeBase::VertexOutput:	return 0;
		case TypeBase::Bool:			return sizeof(TypeBase_t<TypeBase::Bool>);
		case TypeBase::UInt32:			return sizeof(TypeBase_t<TypeBase::UInt32>);
		case TypeBase::Int32:			return sizeof(TypeBase_t<TypeBase::Int32>);
		case TypeBase::UInt64:			return sizeof(TypeBase_t<TypeBase::UInt64>);
		case TypeBase::Int64:			return sizeof(TypeBase_t<TypeBase::Int64>);
		case TypeBase::Int32Vector2:	return sizeof(TypeBase_t<TypeBase::Int32Vector2>);
		case TypeBase::Int32Vector3:	return sizeof(TypeBase_t<TypeBase::Int32Vector3>);
		case TypeBase::Int32Vector4:	return sizeof(TypeBase_t<TypeBase::Int32Vector4>);
		case TypeBase::UInt32Vector2:	return sizeof(TypeBase_t<TypeBase::UInt32Vector2>);
		case TypeBase::UInt32Vector3:	return sizeof(TypeBase_t<TypeBase::UInt32Vector3>);
		case TypeBase::UInt32Vector4:	return sizeof(TypeBase_t<TypeBase::UInt32Vector4>);
		case TypeBase::Int64Vector2:	return sizeof(TypeBase_t<TypeBase::Int64Vector2>);
		case TypeBase::Int64Vector3:	return sizeof(TypeBase_t<TypeBase::Int64Vector3>);
		case TypeBase::Int64Vector4:	return sizeof(TypeBase_t<TypeBase::Int64Vector4>);
		case TypeBase::UInt64Vector2:	return sizeof(TypeBase_t<TypeBase::UInt64Vector2>);
		case TypeBase::UInt64Vector3:	return sizeof(TypeBase_t<TypeBase::UInt64Vector3>);
		case TypeBase::UInt64Vector4:	return sizeof(TypeBase_t<TypeBase::UInt64Vector4>);
		case TypeBase::Float:			return sizeof(TypeBase_t<TypeBase::Float>);
		case TypeBase::FloatVector2:	return sizeof(TypeBase_t<TypeBase::FloatVector2>);
		case TypeBase::FloatVector3:	return sizeof(TypeBase_t<TypeBase::FloatVector3>);
		case TypeBase::FloatVector4:	return sizeof(TypeBase_t<TypeBase::FloatVector4>);
		case TypeBase::FloatMatrix3x3:	return sizeof(TypeBase_t<TypeBase::FloatMatrix3x3>);
		case TypeBase::FloatMatrix4x4:	return sizeof(TypeBase_t<TypeBase::FloatMatrix4x4>);
		case TypeBase::Double:			return sizeof(TypeBase_t<TypeBase::Double>);
		case TypeBase::DoubleVector2:	return sizeof(TypeBase_t<TypeBase::DoubleVector2>);
		case TypeBase::DoubleVector3:	return sizeof(TypeBase_t<TypeBase::DoubleVector3>);
		case TypeBase::DoubleVector4:	return sizeof(TypeBase_t<TypeBase::DoubleVector4>);
		case TypeBase::DoubleMatrix3x3:	return sizeof(TypeBase_t<TypeBase::DoubleMatrix3x3>);
		case TypeBase::DoubleMatrix4x4:	return sizeof(TypeBase_t<TypeBase::DoubleMatrix4x4>);
		case TypeBase::String:			return 8;
		case TypeBase::Texture2D:		return 8;
		case TypeBase::Texture2DArray:	return 8;
		case TypeBase::Other:			return 8;
		case TypeBase::RuntimeArray:	return 0;
		case TypeBase::Sampler:			return 0;
		default:						return 0;
	}
}

static constexpr size_t AlignOfTypeBase(const TypeBase type) {
	switch (type) {
		case TypeBase::None:			return 0;
		case TypeBase::Invalid:			return 0;
		case TypeBase::CBuffer:			return 0;
		case TypeBase::Struct:			return 0;
		case TypeBase::VertexInput:		return 0;
		case TypeBase::VertexOutput:	return 0;
		case TypeBase::Bool:			return alignof(TypeBase_t<TypeBase::Bool>);
		case TypeBase::UInt32:			return alignof(TypeBase_t<TypeBase::UInt32>);
		case TypeBase::Int32:			return alignof(TypeBase_t<TypeBase::Int32>);
		case TypeBase::UInt64:			return alignof(TypeBase_t<TypeBase::UInt64>);
		case TypeBase::Int64:			return alignof(TypeBase_t<TypeBase::Int64>);
		case TypeBase::UInt32Vector2:	return alignof(TypeBase_t<TypeBase::UInt32Vector2>);
		case TypeBase::UInt32Vector3:	return alignof(TypeBase_t<TypeBase::UInt32Vector3>);
		case TypeBase::UInt32Vector4:	return alignof(TypeBase_t<TypeBase::UInt32Vector4>);
		case TypeBase::Int32Vector2:	return alignof(TypeBase_t<TypeBase::Int32Vector2>);
		case TypeBase::Int32Vector3:	return alignof(TypeBase_t<TypeBase::Int32Vector3>);
		case TypeBase::Int32Vector4:	return alignof(TypeBase_t<TypeBase::Int32Vector4>);
		case TypeBase::UInt64Vector2:	return alignof(TypeBase_t<TypeBase::UInt64Vector2>);
		case TypeBase::UInt64Vector3:	return alignof(TypeBase_t<TypeBase::UInt64Vector3>);
		case TypeBase::UInt64Vector4:	return alignof(TypeBase_t<TypeBase::UInt64Vector4>);
		case TypeBase::Int64Vector2:	return alignof(TypeBase_t<TypeBase::Int64Vector2>);
		case TypeBase::Int64Vector3:	return alignof(TypeBase_t<TypeBase::Int64Vector3>);
		case TypeBase::Int64Vector4:	return alignof(TypeBase_t<TypeBase::Int64Vector4>);
		case TypeBase::Float:			return alignof(TypeBase_t<TypeBase::Float>);
		case TypeBase::FloatVector2:	return alignof(TypeBase_t<TypeBase::FloatVector2>);
		case TypeBase::FloatVector3:	return alignof(TypeBase_t<TypeBase::FloatVector3>);
		case TypeBase::FloatVector4:	return alignof(TypeBase_t<TypeBase::FloatVector4>);
		case TypeBase::FloatMatrix3x3:	return alignof(TypeBase_t<TypeBase::FloatMatrix3x3>);
		case TypeBase::FloatMatrix4x4:	return alignof(TypeBase_t<TypeBase::FloatMatrix4x4>);
		case TypeBase::Double:			return alignof(TypeBase_t<TypeBase::Double>);
		case TypeBase::DoubleVector2:	return alignof(TypeBase_t<TypeBase::DoubleVector2>);
		case TypeBase::DoubleVector3:	return alignof(TypeBase_t<TypeBase::DoubleVector3>);
		case TypeBase::DoubleVector4:	return alignof(TypeBase_t<TypeBase::DoubleVector4>);
		case TypeBase::DoubleMatrix3x3:	return alignof(TypeBase_t<TypeBase::DoubleMatrix3x3>);
		case TypeBase::DoubleMatrix4x4:	return alignof(TypeBase_t<TypeBase::DoubleMatrix4x4>);
		case TypeBase::String:			return 8;
		case TypeBase::Texture2D:		return 8;
		case TypeBase::Texture2DArray:	return 8;
		case TypeBase::Other:			return 8;
		case TypeBase::RuntimeArray:	return 0;
		case TypeBase::Sampler:			return 0;
		default:						return 0;
	}
}

static std::pair<size_t, size_t> allocSizeAlignment(const TypeBase type) {

	if (type != TypeBase::Other) {
		return { SizeOfTypeBase(type), AlignOfTypeBase(type)};
	}

	return { sizeof(void*), alignof(void*) };
}

struct VarType
{
private:
	void* _ptr;
	uint64_t _type : 8;
	uint64_t _count : 56;

	

public:
	VarType() = default;
	~VarType() {}

	VarType(void* ptr, const std::vector<VarType>& members) :
		_ptr{ptr},
		_type{static_cast<uint64_t>(TypeBase::Struct)},
		_count{members.size()} {}

	VarType(void* ptr, const TypeBase type, const size_t count) :
		_ptr{ptr},
		_type{static_cast<uint64_t>(type)},
		_count{count} {}

	VarType(TypeBase type, const size_t count, Arena& arena, const size_t alignPad) {
		_type = static_cast<uint64_t>(type);
		_count = count;
		const auto align = AlignOfTypeBase(type);
		_ptr = arena.allocate(
			SizeOfTypeBase(type) * _count,
			alignPad <= align ? align : alignPad);
	}

	template <typename T>
	VarType(const T& value, Arena& arena, const size_t alignPad) {

		if constexpr (is_array_like_v<T>) {
			
			using ElementT = T::value_type;
			_type = static_cast<uint64_t>(TypeBaseFromT<ElementT>());

			if constexpr (is_std_array<T>()) {
				constexpr std::size_t size = array_size_v<T>;
				_count = size;
				const auto [tSize, tAlign] = allocSizeAlignment(static_cast<TypeBase>(_type));
				_ptr = arena.allocate(
					tSize * _count,
					alignPad <= tAlign ? tAlign : alignPad);
				auto* typedPtr = static_cast<ElementT*>(_ptr);
				for (size_t i = 0; i < _count; ++i) {
					typedPtr[i] = value[i];
				}
			} else {
				const std::vector<ElementT>* vector = static_cast<const std::vector<ElementT>*>(&value);
				_count = vector->size();
				auto [tSize, tAlign] = allocSizeAlignment(static_cast<TypeBase>(_type));
				_ptr = arena.allocate(
					tSize * _count,
					alignPad <= tAlign ? tAlign : alignPad);
				ElementT* typedPtr = static_cast<ElementT*>(_ptr);
				for (size_t i = 0; i < _count; ++i) {
					typedPtr[i] = vector->at(i);
				}
			}
		} else {
			_type = static_cast<uint64_t>(TypeBaseFromT<T>());
			_count = 1;
			auto [tSize, tAlign] = allocSizeAlignment(static_cast<TypeBase>(_type));
			_ptr = arena.allocate(
				tSize,
				alignPad <= tAlign ? tAlign : alignPad);
			auto* typedPtr = static_cast<T*>(_ptr);
			*typedPtr = value;
			
		}		
	}

	template <typename T>
	VarType(const T& value, Arena& arena) : VarType{ value, arena, 0 } {}

	VarType(const VarType& rhs) {
		_ptr = rhs._ptr;
		_type = rhs._type;
		_count = rhs._count;
	}
	VarType& operator=(const VarType& rhs) {
		if (this == &rhs)
			return *this;

		_ptr = rhs._ptr;
		_type = rhs._type;
		_count = rhs._count;
		return *this;
	}

	void* allocate(TypeBase type, Arena& arena, const size_t count = 1, const size_t alignPad = 0) {
		_type = static_cast<uint64_t>(type);
		if (TypeBaseIsAllocatable(type)) {
			_count = count;
			auto [tSize, tAlign] = allocSizeAlignment(static_cast<TypeBase>(_type));
			_ptr = arena.allocate(
				tSize * _count,
				alignPad <= tAlign ? tAlign : alignPad);
		} else {
			_ptr = nullptr;
			_count = 0;
		}
		return _ptr;
	}

	void* allocate(Arena& arena, const size_t count = 1, const size_t alignPad = 0) {
		if (TypeBaseIsAllocatable(static_cast<TypeBase>(_type))) {
			_count = count;
			auto [tSize, tAlign] = allocSizeAlignment(static_cast<TypeBase>(_type));
			_ptr = arena.allocate(
				tSize * _count,
				alignPad <= tAlign ? tAlign : alignPad);
		} else {
			_ptr = nullptr;
			_count = 0;
		}
		return _ptr;
	}

	void setType(const TypeBase type) {
		_type = static_cast<uint64_t>(type);
	}
	void setCount(const size_t count) {
		_count = count;
	}

	template <typename T>
	explicit operator T() const {

		if constexpr (TypeBaseFromT<T>() == TypeBase::None)
			throw std::bad_cast();

		return *static_cast<T*>(_ptr);
	}

	template<typename T = void>
	T* data() {
		return static_cast<T*>(_ptr);
	}

	template<typename T = void>
	const T* data() const {
		return static_cast<const T*>(_ptr);
	}

	TypeBase type() {
		return static_cast<TypeBase>(_type);
	}

	template<typename T>
	T* get() {
		if (_type != TypeBaseFromT<T>() || _count != 1) {
			throw std::bad_cast();
		}
		return static_cast<T*>(_ptr);
	}

	size_t size() const { return _count; }

	template<typename T>
	std::span<T> span() {
		return std::span<T>(static_cast<T*>(_ptr), _count);
	}

	template<typename T>
	std::span<const T> span() const {
		return std::span<const T>(static_cast<const T*>(_ptr), _count);
	}

	bool operator==(const VarType& rhs) const {
		return _ptr == rhs._ptr && _type == rhs._type && _count == rhs._count;
	}
	bool operator!=(const VarType& rhs) const {
		return !(*this == rhs);
	}
};






//enum class VarTypeResourceType
//{
//	ConstantBuffer,
//
//};
//
//struct VarTypeStructDefinition
//{
//	struct Member
//	{
//		//MatVar var;
//		uint32_t nameIndex;		
//		uint8_t globalSetVar = 0;		
//		uint8_t alignment;
//		uint32_t count;		
//		uint32_t offset;
//	};
//
//	std::vector<Member> members;
//
//	void finalizeLayout() {
//		size_t offset = 0;
//		for (auto& m : members) {
//			size_t naturalAlign = std::max(alignOfTypeVar(m.var.type()), static_cast<size_t>(m.alignment));
//			offset = (offset + naturalAlign - 1) & ~(naturalAlign - 1); // align up
//			m.offset = offset;
//			offset += sizeOfTypeVar(m.var.type()) * m.count;
//		}
//	}
//
//	uint32_t sizeBytes() const {
//		if (members.empty()) return 0;
//		auto& last = members.back();
//		return static_cast<uint32_t>(last.offset + sizeOfTypeVar(last.var.type()) * last.count);
//	}
//};
//
//
//struct VarTypeStruct
//{
//private:
//	void* _ptr = nullptr;
//	VarTypeStructDefinition* const _definition;
//
//public:
//	VarTypeStruct() = delete;
//	VarTypeStruct(VarTypeStructDefinition* const def, Arena& arena) :
//		_definition{def}
//	{
//		
//	}
//
//	void* data() { return _ptr; }
//	size_t sizeInBytes() const { return static_cast<size_t>(_definition->sizeBytes()); }
//	size_t members() const { return _definition->members.size(); }
//	VarType getField(size_t index) const {
//		auto& m = _definition->members[index];
//		return VarType{
//			static_cast<uint8_t*>(_ptr) + m.offset,
//			m.var.type(),
//			m.count
//		};
//	}
//};

#endif