#include "Shader.h"
#include "ShaderManager.h"

Shader::Shader(Type type, const std::filesystem::path& path) :
    type(type),
    sourcePath(path),
    m_arrayIndex{ static_cast<size_t>(-1) }
{
    lastSourceChangedTime = std::filesystem::last_write_time(sourcePath);
}

const std::string& Shader::name() {
    return ShaderManager::getInstance()->getShaderName(*this);
}

void Shader::reflect() {
    auto result = reflectShader(bytecode);
    parameters = result.first;
    pushConstants = result.second;
}

std::pair<std::vector<
    ShaderParameter>, std::vector<ShaderPushConstant>
> Shader::reflectShader(const std::vector<uint32_t>& spirv)
{
    SpvReflectShaderModule module;
    spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module);

    VkShaderStageFlagBits stage = static_cast<VkShaderStageFlagBits>(module.shader_stage);
    uint32_t count = 0;
    spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
    uint32_t pcCount = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
    std::vector<SpvReflectBlockVariable*> pConstants(pcCount);
    spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pConstants.data());
    std::vector<ShaderParameter> params;
    for (auto* b : bindings) {
        if (!b) continue;

        // Only go deeper for UBOs or SSBOs
        if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {

            const SpvReflectBlockVariable* block = &b->block;

            for (uint32_t i = 0; i < block->member_count; ++i) {
                const SpvReflectBlockVariable& member = block->members[i];

                ShaderParameter p;
                p.name = member.name;
                p.set = b->set;
                p.binding = b->binding;
                p.stageFlags = stage;
                p.count = member.array.dims_count > 0 ? member.array.dims[0] : 1;
                p.descriptorType = mapReflectToVkDescriptorType(b->descriptor_type);
                p.offset = member.offset;

                params.push_back(p);
            }

        } else {
            ShaderParameter p;
            p.name = b->name;
            p.set = b->set;
            p.binding = b->binding;
            p.stageFlags = stage;
            p.count = b->count;
            p.descriptorType = mapReflectToVkDescriptorType(b->descriptor_type);
            p.offset = 0;

            params.push_back(p);
        }
    }
    std::vector<ShaderPushConstant> consts;
    for (auto* pc : pConstants) {
        if (!pc) continue;
        ShaderPushConstant p;
        p.name = pc->name;
        p.offset = pc->offset;
        p.size = pc->size;
        p.stageFlags = stage;

        consts.push_back(p);
    }


    spvReflectDestroyShaderModule(&module);
    return { params, consts };
}

std::vector<uint32_t> Shader::toUint32Vector(const std::vector<char>& charVec) {
    size_t wordCount = charVec.size() / sizeof(uint32_t);
    std::vector<uint32_t> out(wordCount);
    std::memcpy(out.data(), charVec.data(), wordCount * sizeof(uint32_t));
    return out;
}