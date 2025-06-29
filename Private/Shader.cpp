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

ErrorCode Shader::reflect() {
    try {
        auto result = reflectShader(bytecode);
        parameters = std::get<0>(result);
        pushConstants = std::get<1>(result);
        iaInput = std::get<2>(result);
    } catch (std::exception& e) {
        return ErrorCode::SHADER_REFLECTION_ERROR;
    }
    return ErrorCode::OK;
}

std::tuple<
    std::vector<ShaderBinding>, std::vector<ShaderPushConstant>, ShaderInput
> Shader::reflectShader(const std::vector<uint32_t>& spirv)
{
    SpvReflectShaderModule module;
    spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module);

    VkShaderStageFlagBits stage = static_cast<VkShaderStageFlagBits>(module.shader_stage);
    uint32_t count = 0;

    spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());

    spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
    std::vector<SpvReflectBlockVariable*> pConstants(count);
    spvReflectEnumeratePushConstantBlocks(&module, &count, pConstants.data());

    spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorSet*> descSets(count);
    spvReflectEnumerateDescriptorSets(&module, &count, descSets.data());

    spvReflectEnumerateInputVariables(&module, &count, nullptr);
    std::vector<SpvReflectInterfaceVariable*> inputVars(count);
    spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
    ShaderInput ia;
    for (auto& var : inputVars) {
        ShaderInput::Attribute attrib{};
        attrib.location = var->location;
        //attrib.spvTypeDesc = *var->type_description;
        attrib.type = parseReflectedTypeDesc(var->type_description);
        ia.attributes.push_back(attrib);
    }

    std::vector<ShaderBinding> params;
    for (auto* b : bindings) {
        if (!b) continue;

        ShaderBinding p;
        p.name = b->name;
        p.set = b->set;
        p.binding = b->binding;
        p.stageFlags = stage;
        p.count = b->count;
        p.spvTypeDesc = b->type_description ? *b->type_description : SpvReflectTypeDescription{};
        p.descriptorType = mapReflectToVkDescriptorType(b->descriptor_type);
        p.offset = 0;
        
        const SpvReflectBlockVariable* block = &b->block;
        for (uint32_t i = 0; i < block->member_count; ++i) {
            const SpvReflectBlockVariable& member = block->members[i];
            p.members.push_back(parseMember(
                member, b->set, b->binding, stage, mapReflectToVkDescriptorType(b->descriptor_type)));
        }
        params.push_back(p);
    }

    std::vector<ShaderPushConstant> consts;
    for (auto* pc : pConstants) {
        if (!pc) continue;

        ShaderPushConstant pcParam;
        pcParam.name = pc->name;
        pcParam.offset = pc->offset;
        pcParam.size = pc->size;
        pcParam.stageFlags = stage;
        pcParam.spvTypeDesc = pc->type_description ? *pc->type_description : SpvReflectTypeDescription{};

        for (uint32_t i = 0; i < pc->member_count; ++i) {
            pcParam.members.push_back(
                parseMember(pc->members[i], 0, 0xFFFFFFFF, stage, VK_DESCRIPTOR_TYPE_MAX_ENUM)
            );
        }

        consts.push_back(pcParam);
    }

    spvReflectDestroyShaderModule(&module);
    return std::make_tuple( params, consts, ia );
}


ShaderBinding Shader::parseMember(
    const SpvReflectBlockVariable& member,
    uint32_t set,
    uint32_t binding,
    VkShaderStageFlags stageFlags,
    VkDescriptorType descriptorType)
{
    ShaderBinding param;
    param.name = member.name ? member.name : "";
    param.spvTypeDesc = member.type_description ? *member.type_description : SpvReflectTypeDescription{};
    param.set = set;
    param.binding = binding;
    param.stageFlags = stageFlags;
    param.descriptorType = descriptorType;
    param.count = (member.array.dims_count > 0) ? member.array.dims[0] : 1;
    param.offset = member.offset;

    // Check for nested struct
    for (uint32_t i = 0; i < member.member_count; ++i) {
        param.members.push_back(
            parseMember(member.members[i], set, binding, stageFlags, descriptorType)
        );
    }
    
    return param;
}