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


struct ShaderParameter
{
    std::string name;
    SpvReflectTypeDescription spvTypeDesc;
    uint32_t set;
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
    uint32_t count = 1;
    uint32_t offset = 0;
    std::vector<ShaderParameter> members;
};

struct ShaderPushConstant
{
    std::string name;
    SpvReflectTypeDescription spvTypeDesc;
    uint32_t offset;
    uint32_t size;
    std::vector<ShaderParameter> members;
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
    std::vector<ShaderParameter> parameters;
    std::vector<ShaderPushConstant> pushConstants;

    Shader(Type type, const std::filesystem::path& path);

    const std::string& name();

    ErrorCode reflect();

    static std::pair<
        std::vector<ShaderParameter>, std::vector<ShaderPushConstant>
    > reflectShader(const std::vector<uint32_t>& spirv);

    //static std::vector<uint32_t> toUint32Vector(const std::vector<char>& charVec);
    //inline static std::pair<std::vector<ShaderParameter>, std::vector<ShaderPushConstant>> reflectShader(
    //    const std::vector<char>& spirv) { return reflectShader(toUint32Vector(spirv)); }

#ifdef USE_VULKAN
    inline static VkDescriptorType mapReflectToVkDescriptorType(SpvReflectDescriptorType reflectType) {
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


    inline static MatParamType parseReflectedTypeDesc(const SpvReflectTypeDescription* typeDesc) {
        auto param = MatParamType{ MatParamType::Base::Invalid };
        
        if (!typeDesc)
            return param;

        SpvReflectNumericTraits numeric = typeDesc->traits.numeric;

        switch (typeDesc->op) {
            case SpvOp::SpvOpTypeStruct:
            {
                param.base = MatParamType::Base::Struct;
            } break;

            case SpvOp::SpvOpTypeSampler:
            {
                param.base = MatParamType::Base::Sampler;
            } break;

            case SpvOp::SpvOpTypeRuntimeArray:
            {
                param.base = MatParamType::Base::RuntimeArray;
            } break;

            case SpvOp::SpvOpTypeMatrix:
            {
                param.rows = numeric.matrix.row_count;
                param.cols = numeric.matrix.column_count;
                if (numeric.scalar.width == 32) param.base = MatParamType::Base::FloatMatrix;
                else if (numeric.scalar.width == 64) param.base = MatParamType::Base::DoubleMatrix;
            } break;

            case SpvOp::SpvOpTypeVector:
            {
                param.rows = numeric.vector.component_count;
                if (numeric.scalar.width == 32) param.base = MatParamType::Base::FloatVector;
                else if (numeric.scalar.width == 64) param.base = MatParamType::Base::DoubleVector;
            } break;

            case SpvOp::SpvOpTypeBool:
            {
                param.base = MatParamType::Base::Bool;
            } break;

            case SpvOp::SpvOpTypeFloat:
            {
                if (numeric.scalar.width == 32) param.base = MatParamType::Base::Float;
                else if (numeric.scalar.width == 64) param.base = MatParamType::Base::Double;
            } break;

            case SpvOp::SpvOpTypeInt:
            {
                if (numeric.scalar.width <= 32) {
                    param.base = numeric.scalar.signedness ? MatParamType::Base::Int32 : MatParamType::Base::UInt32;
                } else if (numeric.scalar.width == 64) {
                    param.base = numeric.scalar.signedness ? MatParamType::Base::Int64 : MatParamType::Base::UInt64;
                }
            } break;


        }

        return param;
    }

    static ShaderParameter parseMember(const SpvReflectBlockVariable& member,
                                       uint32_t set, uint32_t binding,
                                       VkShaderStageFlags stageFlags,
                                       VkDescriptorType descriptorType);


#endif // USE_VULKAN


};

#endif // ! SHADER_HPP