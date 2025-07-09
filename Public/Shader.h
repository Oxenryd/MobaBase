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

#ifdef USE_VULKAN
    #ifndef VULKAN_CORE_H_
        #include <vulkan/vulkan_core.h>
    #endif
#include <spirv_reflect.h>



struct ShaderInput
{
    struct Attribute
    {
        uint16_t offset;
        uint8_t location;
        TypeBase type;
        //SpvReflectTypeDescription spvTypeDesc;
    };
    std::vector<Attribute> attributes;
};

struct ShaderBinding
{
    std::string name;
    SpvReflectTypeDescription spvTypeDesc;
    uint32_t set;
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
    uint32_t count = 1;
    uint32_t offset = 0;
    std::vector<ShaderBinding> members;
};

struct ShaderPushConstant
{
    std::string name;
    SpvReflectTypeDescription spvTypeDesc;
    uint32_t offset;
    uint32_t size;
    std::vector<ShaderBinding> members;
    VkShaderStageFlags stageFlags;

    VkPushConstantRange toVkRange() const {
        return VkPushConstantRange{
            .stageFlags = stageFlags,
            .offset = offset,
            .size = size
        };
    }
};

#endif // USE_VULKAN

class ShaderManager;
class Shader
{
private:
    friend ShaderManager;
    size_t m_arrayIndex;
public:
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
    ShaderInput iaInput;
    std::vector<ShaderBinding> parameters;
    std::vector<ShaderPushConstant> pushConstants;

    Shader(Type type, const std::filesystem::path& path);

    const std::string& name();

    ErrorCode reflect();

    static std::tuple<
        std::vector<ShaderBinding>, std::vector<ShaderPushConstant>, ShaderInput
    > reflectShader(const std::vector<uint32_t>& spirv);

    //static std::vector<uint32_t> toUint32Vector(const std::vector<char>& charVec);
    //inline static std::pair<std::vector<ShaderParameter>, std::vector<ShaderPushConstant>> reflectShader(
    //    const std::vector<char>& spirv) { return reflectShader(toUint32Vector(spirv)); }

#ifdef USE_VULKAN
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
        const SpvReflectTypeDescription* typeDesc, const VkDescriptorType* vkDescType) {
        
        if (!typeDesc)
            return TypeBase::Invalid;

        SpvReflectNumericTraits numeric = typeDesc->traits.numeric;

        switch (typeDesc->op) {
            case SpvOp::SpvOpTypeStruct:
            {
                if (vkDescType) {
                    switch (*vkDescType) {
                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                            return TypeBase::CBuffer;

                        case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                            return TypeBase::StructBuffer;
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
                auto& rows = numeric.matrix.row_count;
                auto& cols = numeric.matrix.column_count;

                switch (cols) {
                    case 3:
                    {
                        switch (rows) {
                            case 3:
                            {
                                if (numeric.scalar.width == 32) return TypeBase::FloatMatrix3x3;
                                else return TypeBase::DoubleMatrix3x3;
                            } 
                        }
                    } break;

                    case 4:
                    {
                        switch (rows) {
                            case 4:
                            {
                                if (numeric.scalar.width == 32) return TypeBase::FloatMatrix4x4;
                                else return TypeBase::DoubleMatrix4x4;
                            }
                        }
                    } break;
                }

            } break;

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

                bool isFloating = typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT;

                if (!isFloating)
                {
                    switch (numeric.scalar.signedness)
                    {

                        case UInt:
                        {
                            switch (numeric.vector.component_count) {
                                default:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32;
                                    else return TypeBase::UInt64;
                                }

                                case 2:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector2;
                                    else return TypeBase::UInt64Vector2;
                                }

                                case 3:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector3;
                                    else return TypeBase::UInt64Vector3;
                                }

                                case 4:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::UInt32Vector4;
                                    else return TypeBase::UInt64Vector4;
                                }
                            }
                        }

                        case Int:
                        {
                            switch (numeric.vector.component_count) {
                                default:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32;
                                    else return TypeBase::Int64;
                                }

                                case 2:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector2;
                                    else return TypeBase::Int64Vector2;
                                }

                                case 3:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector3;
                                    else return TypeBase::Int64Vector3;
                                }

                                case 4:
                                {
                                    if (numeric.scalar.width <= 32) return TypeBase::Int32Vector4;
                                    else return TypeBase::Int64Vector4;
                                }
                            }
                        }
                    }
                } else {
                    switch (numeric.vector.component_count) {
                        default:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::Float;
                            else return TypeBase::Double;
                        }

                        case 2:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector2;
                            else return TypeBase::DoubleVector2;
                        }

                        case 3:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector3;
                            else return TypeBase::DoubleVector3;
                        }

                        case 4:
                        {
                            if (numeric.scalar.width <= 32) return TypeBase::FloatVector4;
                            else return TypeBase::DoubleVector4;
                        }
                    }
                }

            } break;

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
        return TypeBase::None;
    }

    static ShaderBinding parseMember(const SpvReflectBlockVariable& member,
                                       uint32_t set, uint32_t binding,
                                       VkShaderStageFlags stageFlags,
                                       VkDescriptorType descriptorType);


#endif // USE_VULKAN


};

#endif // ! SHADER_HPP