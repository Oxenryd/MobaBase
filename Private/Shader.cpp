#include "Shader.h"
#include "RenderManager.h"

Shader::Shader(Type type, const std::filesystem::path& path) :
    type(type),
    sourcePath(path),
    m_arrayIndex{ static_cast<size_t>(-1) }
{
    lastSourceChangedTime = std::filesystem::last_write_time(sourcePath);
}

const std::string& Shader::name() {
    return RenderManager::getInstance()->getShaderName(*this);
}

ErrorCode Shader::reflect() {
    try {
        auto result = reflectShader(bytecode);
        entryPoint = std::get<0>(result);
        parameters = std::get<1>(result);
        pushConstants = std::get<2>(result);
        input = std::get<3>(result);
        output = std::get<4>(result);
    } catch (std::exception& e) {
        return ErrorCode::SHADER_REFLECTION_ERROR;
    }
    return ErrorCode::OK;
}

std::tuple<
    std::string, std::vector<ShaderBinding>, std::vector<ShaderPushConstant>, ShaderIO, ShaderIO
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
    ShaderIO shaderInput;
    size_t iaOffset = 0;
    for (auto& var : inputVars) {
        ShaderIO::Attribute attrib{};
        attrib.location = var->location;
        attrib.type = parseReflectedTypeDesc(var->type_description, nullptr);
        attrib.offset = static_cast<uint16_t>(iaOffset);

        size_t paramSize = SizeOfTypeBase(attrib.type);
        //size_t paramAlign = AlignOfTypeBase(attrib.type);      

        size_t vkAlignment = 0;
        size_t alignTest = 2;
        size_t counter = 1;
        do {
            alignTest *= 2;
            vkAlignment = alignTest;
        } while (paramSize > vkAlignment);
        iaOffset += vkAlignment;

        if (var->name) {
            std::string s{ var->name };
            size_t lastDot = s.rfind('.');
            std::string lastWord = (lastDot == std::string::npos) ? s : s.substr(lastDot + 1);

            auto semNameIndex = RenderManager::getInstance()->getParamNameIndex(lastWord);
            if (semNameIndex == SIZE_INVALID) {
                attrib.semanticNameIndex = RenderManager::getInstance()->registerParamName(lastWord);
            } else {
                attrib.semanticNameIndex = semNameIndex;
            }
        }


        shaderInput.attributes.push_back(attrib);
    }

    spvReflectEnumerateOutputVariables(&module, &count, nullptr);
    std::vector<SpvReflectInterfaceVariable*> outputVars(count);
    spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
    ShaderIO shaderOutput;
    for (auto& var : outputVars) {
        ShaderIO::Attribute attrib{};
        attrib.location = var->location;
        attrib.type = parseReflectedTypeDesc(var->type_description, nullptr);

        if (var->name) {
            std::string s{ var->name };
            size_t lastDot = s.rfind('.');
            std::string lastWord = (lastDot == std::string::npos) ? s : s.substr(lastDot + 1);

            auto semNameIndex = RenderManager::getInstance()->getParamNameIndex(lastWord);
            if (semNameIndex == SIZE_INVALID) {
                attrib.semanticNameIndex = RenderManager::getInstance()->registerParamName(lastWord);
            } else {
                attrib.semanticNameIndex = semNameIndex;
            }
        }

        shaderOutput.attributes.push_back(attrib);
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
        p.spvTypeDesc = b->type_description ? SpvReflectTypeDescription{ *b->type_description } : SpvReflectTypeDescription{};
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
        auto nameIndex = RenderManager::getInstance()->getParamNameIndex(pc->name);
        if (nameIndex == SIZE_INVALID) {
            pcParam.base.nameIndex = RenderManager::getInstance()->registerParamName(pc->name);
        } else {
            pcParam.base.nameIndex = nameIndex;
        }

        if (pc->member_count == 0) { // single variable

            pcParam.base.size = pc->padded_size;
            pcParam.base.offset = pc->offset;
            pcParam.base.type = TypeBase::PushConst;
            ShaderPushConstant::Attribute attrib{};
            attrib.offset = pc->offset;
            attrib.size = pc->padded_size;
            attrib.type = parseReflectedTypeDesc(pc->type_description, nullptr);
            attrib.nameIndex = pcParam.base.nameIndex;
            pcParam.members.push_back(attrib);
            consts.push_back(pcParam);
        } else {
            pcParam.base.offset = pc->offset;
            pcParam.base.size = pc->size;
            pcParam.stageFlags = stage;
            pcParam.base.type = TypeBase::PushConstStruct;

            for (uint32_t i = 0; i < pc->member_count; ++i) {
                auto& member = pc->members[i];
                ShaderPushConstant::Attribute attrib{};
                attrib.offset = member.offset;
                attrib.size = member.padded_size;
                attrib.type = parseReflectedTypeDesc(member.type_description, nullptr);

                auto nameIndex = RenderManager::getInstance()->getParamNameIndex(member.name);
                if (nameIndex == SIZE_INVALID) {
                    attrib.nameIndex = RenderManager::getInstance()->registerParamName(member.name);
                } else {
                    attrib.nameIndex = nameIndex;
                }
                pcParam.members.push_back(attrib);
            }

            consts.push_back(pcParam);
        }
    }
    std::string entry = module.entry_point_name;
    spvReflectDestroyShaderModule(&module);
    return std::make_tuple(entry, params, consts, shaderInput, shaderOutput );
}


ShaderBinding Shader::parseMember(
    const SpvReflectBlockVariable& member,
    uint32_t set,
    uint32_t binding,
    VkShaderStageFlags stageFlags,
    VkDescriptorType descriptorType)
{
    ShaderBinding param;
    param.spvTypeDesc = member.type_description ? SpvReflectTypeDescription{ *member.type_description } : SpvReflectTypeDescription{};
    if (member.name != nullptr)
        param.name = member.name;
    else if (param.spvTypeDesc.op == SpvOpTypeRuntimeArray || param.spvTypeDesc.op == SpvOpTypeArray) {
        if (param.spvTypeDesc.type_name != nullptr)
            param.name = param.spvTypeDesc.type_name;
        else param.name = "";
    } else
        param.name = "";
    
    //param.name = member.name != nullptr
    //    ? member.name
    //    : param.spvTypeDesc.op == SpvOpTypeRuntimeArray || param.spvTypeDesc.op == SpvOpTypeArray
    //        ? (param.spvTypeDesc.type_name != nullptr 
    //        ? param.spvTypeDesc.type_name
    //        : "");
    
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


std::string& ShaderIO::Attribute::semantic() const {
    return RenderManager::getInstance()->getParamName(semanticNameIndex);
}


std::string& ShaderPushConstant::Attribute::name() const {
    return RenderManager::getInstance()->getParamName(nameIndex);
}