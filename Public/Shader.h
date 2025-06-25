#ifndef SHADER_HPP
#define SHADER_HPP

#include <filesystem>
#include <cstdint>
#include <chrono>
#include <vector>
#include <utility>

#include "Bits.hpp"

#ifdef USE_VULKAN
    #ifndef VULKAN_CORE_H_
        #include <vulkan/vulkan_core.h>
    #endif
#include <spirv_reflect.h>

struct ShaderParameter
{
    std::string name;
    uint32_t set;
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
    uint32_t count = 1;
    uint32_t offset = 0;
};

struct ShaderPushConstant
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    VkShaderStageFlags stageFlags;

    VkPushConstantRange toVkRange() const {
        return VkPushConstantRange{
            .stageFlags = stageFlags,
            .offset = offset,
            .size = size
        };
    }
};

#endif
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
    std::vector<char> bytecode;
    std::vector<ShaderParameter> parameters;
    std::vector<ShaderPushConstant> pushConstants;

    Shader(Type type, const std::filesystem::path& path);

    const std::string& name();

    void reflect();

    static std::pair<
        std::vector<ShaderParameter>, std::vector<ShaderPushConstant>
    > reflectShader(const std::vector<uint32_t>& spirv);

    static std::vector<uint32_t> toUint32Vector(const std::vector<char>& charVec);
    inline static std::pair<std::vector<ShaderParameter>, std::vector<ShaderPushConstant>> reflectShader(
        const std::vector<char>& spirv) { return reflectShader(toUint32Vector(spirv)); }

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

#endif
};

#endif // ! SHADER_HPP