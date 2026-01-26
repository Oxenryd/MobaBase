#ifndef SHADER_HPP
#define SHADER_HPP

#include <filesystem>
#include <cstdint>
#include <chrono>
#include <vector>
#include <utility>

#include "ErrorCodes.hpp"
#include "Bits.hpp"
#include "MaterialParams.h"

#ifndef VULKAN_CORE_H_
    #include <vulkan/vulkan_core.h>
#endif
#include <spirv_reflect.h>


struct ShaderIO
{
    struct Attribute
    {
        uint32_t semanticNameIndex;
        uint16_t offset;
        uint8_t location;
        TypeBase type;
        std::string& semantic() const;
    };
    std::vector<Attribute> attributes;
};

struct ShaderBinding
{
    std::string name;
    SpvReflectTypeDescription spvTypeDesc;
    SpvReflectResourceType spvResourceType{};
    uint32_t set{};
    uint32_t binding{};
    VkDescriptorType descriptorType{};
    VkShaderStageFlags stageFlags{};
    uint32_t count = 1;
    uint32_t offset = 0;
    std::vector<ShaderBinding> members;

};

struct ShaderPushConstant
{
    struct Attribute
    {
        uint32_t nameIndex = UINT32_INVALID;
        uint16_t offset = UINT16_INVALID;
        uint16_t size{};
        TypeBase type{};
        std::string& name() const;
    };
    Attribute base;
    VkShaderStageFlags stageFlags;
    std::vector<Attribute> members;

    VkPushConstantRange toVkRange() const {
        return VkPushConstantRange{
            .stageFlags = stageFlags,
            .offset = base.offset,
            .size = base.size
        };
    }
};

class RenderManager;
class Shader
{
private:
    friend RenderManager;
    size_t m_arrayIndex;
public:
    std::string entryPoint;
    enum class Type : uint8_t
    {
        Invalid,
        Vertex,
        Fragment,
        Compute
    };
    Type type;
    std::filesystem::path sourcePath;
    std::filesystem::file_time_type lastSourceChangedTime;
    std::vector<uint32_t> bytecode;
    ShaderIO input;
    ShaderIO output;
    std::vector<ShaderBinding> parameters;
    std::vector<ShaderPushConstant> pushConstants;

    Shader(Type type, const std::filesystem::path& path);

    const std::string& name();

    ErrorCode reflect();

    static std::tuple<
        std::string, std::vector<ShaderBinding>, std::vector<ShaderPushConstant>, ShaderIO, ShaderIO
    > reflectShader(const std::vector<uint32_t>& spirv);


    inline static VkDescriptorType mapReflectToVkDescriptorType(
        SpvReflectDescriptorType reflectType) {
        switch (reflectType) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            default:
                throw std::runtime_error("Unknown SPIRV descriptor type");
        }
    }


    inline static TypeBase parseReflectedTypeDesc(
        const SpvReflectTypeDescription* typeDesc, const VkDescriptorType* vkDescType, uint32_t resourceFlags) {
        
        if (!typeDesc)
            return TypeBase::Invalid;

        SpvReflectNumericTraits numeric = typeDesc->traits.numeric;

        switch (typeDesc->op) {
            default: return TypeBase::Invalid;
            case SpvOp::SpvOpTypeStruct:
            {
                if (vkDescType) {
                    switch (*vkDescType) {
                        default: return TypeBase::Invalid;
                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                            return TypeBase::CBuffer;

                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        {
                            if (resourceFlags & MatParam::UAV)
                                return TypeBase::RWStructBuffer;
                            else
                                return TypeBase::StructBuffer;
                        }
                    }
                }
                return TypeBase::Struct;
            } 

            case SpvOp::SpvOpTypeSampler:
            {
                return TypeBase::Sampler;
            } 

            case SpvOp::SpvOpTypeRuntimeArray:
            {
                if (vkDescType) {
                    switch (*vkDescType) {
                        default: return TypeBase::Invalid;
                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                            return TypeBase::Texture2DArray;

                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        {
                            return TypeBase::Struct;
                        }
                    }
                }

                return TypeBase::RuntimeArray;
            }

            case SpvOp::SpvOpTypeMatrix:
            {
                const auto& rows = numeric.matrix.row_count;
                switch (const auto& cols = numeric.matrix.column_count) {
#ifdef BUILD_WIN
                    default: std::unreachable();
#else
                    default: __builtin_unreachable();
#endif
                    
                    case 2:
                    {
                        if (rows == 2) {
                            if (numeric.scalar.width == 32) return TypeBase::FloatMatrix2x2;
                            return TypeBase::DoubleMatrix2x2;
                        }
                        return TypeBase::UnimplementedMatrix;
                    } break;
                    case 3:
                    {
                        if (rows == 3) {
                            if (numeric.scalar.width == 32) return TypeBase::FloatMatrix3x3;
                            return TypeBase::DoubleMatrix3x3;
                        }
                        return TypeBase::UnimplementedMatrix;
                    }

                    case 4:
                    {
                        if (rows == 4) {
                            if (numeric.scalar.width == 32) return TypeBase::FloatMatrix4x4;
                            return TypeBase::DoubleMatrix4x4;
                        }
                        return TypeBase::UnimplementedMatrix;
                    }
                }
            }

            case SpvOp::SpvOpTypeFloat:
            {
                if (numeric.scalar.width <= 32) {
                    return  TypeBase::Float;
                } else {
                    return TypeBase::Double;
                }
            }

            case SpvOp::SpvOpTypeVector:
            {
                enum
                {
                    UInt = 0,
                    Int = 1,
                };

                if (const bool isFloating = typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT;
                    !isFloating)
                {
                    switch (numeric.scalar.signedness)
                    {
                        default:
                        case UInt:
                        {
                            switch (numeric.vector.component_count) {
                                default:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32;
                                    return TypeBase::UInt64;
                                }

                                case 2:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector2;
                                    return TypeBase::UInt64Vector2;
                                }

                                case 3:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector3;
                                    return TypeBase::UInt64Vector3;
                                }

                                case 4:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector4;
                                    return TypeBase::UInt64Vector4;
                                }
                            }
                        }

                        case Int:
                        {
                            switch (numeric.vector.component_count) {
                                default:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32;
                                    return TypeBase::Int64;
                                }

                                case 2:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector2;
                                    return TypeBase::Int64Vector2;
                                }

                                case 3:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector3;
                                    return TypeBase::Int64Vector3;
                                }

                                case 4:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector4;
                                    return TypeBase::Int64Vector4;
                                }
                            }
                        }
                    }
                } else {
                    switch (numeric.vector.component_count) {
                        default:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::Float;
                            return TypeBase::Double;
                        }

                        case 2:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector2;
                            return TypeBase::DoubleVector2;
                        }

                        case 3:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector3;
                            return TypeBase::DoubleVector3;
                        }

                        case 4:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector4;
                            return TypeBase::DoubleVector4;
                        }
                    }
                }
            }

            case SpvOp::SpvOpTypeBool:
            {
                return TypeBase::Bool;
            } 


            case SpvOp::SpvOpTypeInt:
            {
                if (numeric.scalar.width <= 32) {
                    return  numeric.scalar.signedness ? TypeBase::Int32 : TypeBase::UInt32;
                } else  {
                    return  numeric.scalar.signedness ? TypeBase::Int64 : TypeBase::UInt64;
                }
            }
        }
    }

    inline static VkDescriptorType parseMatParamToVkDescriptorType(const MatParam& param) {

        switch (param.type) {
            case TypeBase::Struct:
            case TypeBase::StructBuffer: 
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            case TypeBase::Texture2D:
            case TypeBase::Texture2DArray:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            case TypeBase::Sampler:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;

            default:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }

        throw std::invalid_argument("MatParam param does not match any Vulkan Descriptor type.");
    }




    static ShaderBinding parseMember(const SpvReflectBlockVariable& member,
                                       uint32_t set, uint32_t binding,
                                       VkShaderStageFlags stageFlags,
                                       VkDescriptorType descriptorType);


};

#endif // ! SHADER_HPP