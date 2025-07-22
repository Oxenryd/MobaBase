#include "Material.hpp"
#include "Shader.h"
#include "RenderManager.h"
#include "Hashes.hpp"

#include <iostream>
#include <format>

//json Material::toJson() {
//	json j;
//	j["name"] = name();
//	j["vShaderName"] = vShaderName;
//	j["pShaderName"] = pShaderName;
//
//	j["params"] = json::array();
//	for (auto& p : paramsMap)
//		j["params"].push_back(p.second.toJson());
//
//	return j;
//}
//
//void Material::fromJson(const json& j) {
//	auto newName = j.at("name").get<std::string>();
//	vShaderName = j.at("vShaderName").get<std::string>();
//	pShaderName = j.at("pShaderName").get<std::string>();
//
//	paramsMap.clear();
//	for (const auto& jp : j.at("params")) {
//		MatParam param;
//		param.fromJson(jp);
//		paramsMap.insert({param.name, std::move(param) });
//	}
//	ShaderManager::getInstance()->registerMaterial(newName, *this);
//}

void Material::addShaderParams(
	std::vector<MatParam>& params,
	std::vector<ShaderBinding>& s,
	MatParamStage stage,
	const std::string& parentName)
{
	for (ShaderBinding& param : s) {
		MatParam matParam{};
		matParam.type = Shader::parseReflectedTypeDesc(&param.spvTypeDesc, &param.descriptorType);
		matParam.stage = stage;
		matParam.descriptorType = param.descriptorType;
		if (param.spvTypeDesc.op == SpvOpTypeRuntimeArray) {
			if (param.count == 0) {
				matParam.arrayType = MatParamArrayType::Dynamic;
				if (matParam.type == TypeBase::Texture2DArray)
					matParam.count = VULKAN_MAX_TEXTURE_SSBO;
				else
					matParam.count = VULKAN_MAX_RUNTIMEARRAY_INSTANCES;
			} else {
				matParam.arrayType = MatParamArrayType::Static;
				matParam.count = param.count;
			}
		} else
			matParam.count = param.count;
		matParam.bindingIndex = param.binding;
		matParam.setIndex = param.set;
		matParam.offset = param.offset;

		std::string name;
		auto nameIndex = RenderManager::getInstance()->getParamNameIndex(param.name);
		if (nameIndex == SIZE_T_INVALID) {
			name = param.name;
			matParam.nameIndex = RenderManager::getInstance()->registerParamName(name);
		} else {
			matParam.nameIndex = nameIndex;
			name = RenderManager::getInstance()->getParamName(nameIndex);
		}


		if (!parentName.empty()) {
			auto parentNameIndex = RenderManager::getInstance()->getParamNameIndex(parentName);
			if (parentNameIndex == SIZE_T_INVALID) {
				matParam.parentNameIndex = RenderManager::getInstance()->registerParamName(parentName);
			} else {
				matParam.parentNameIndex = parentNameIndex;
			}
		} else
			matParam.parentNameIndex = UINT32_INVALID;


		addShaderParams(matParam.members, param.members, stage, name);
		
		params.push_back(matParam);
	}
}

Material& Material::initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment) {
	Material material{};
	auto& this_ = *RenderManager::getInstance()->registerMaterial(newName, material);
	this_.vShaderName = vertex.name();
	this_.pShaderName = fragment.name();

	std::vector<MatParam> vsParams;	
	this_.addShaderParams(vsParams, vertex.parameters, MatParamStage::Vertex, "");
	for (auto& pConst : vertex.pushConstants) {
		MatParam pconstParam{};
		pconstParam.parentNameIndex = UINT32_INVALID;
		pconstParam.type = pConst.base.type;
		pconstParam.stage = MatParamStage::Vertex;
		pconstParam.offset = pConst.base.offset;
		pconstParam.size = pConst.base.size;
		pconstParam.nameIndex = pConst.base.nameIndex;
		for (auto& member : pConst.members) {
			MatParam pconstParamMember{};
			pconstParamMember.parentNameIndex = pconstParam.nameIndex;
			pconstParamMember.type = member.type;
			pconstParamMember.stage = MatParamStage::Vertex;
			pconstParamMember.offset = member.offset;
			pconstParamMember.nameIndex = member.nameIndex;
			pconstParam.members.push_back(pconstParamMember);
		}
		vsParams.push_back(pconstParam);
	}

	std::vector<MatParam> psParams;
	this_.addShaderParams(psParams, fragment.parameters, MatParamStage::Fragment, "");
	for (auto& pConst : fragment.pushConstants) {
		MatParam pconstParam{};
		pconstParam.parentNameIndex = UINT32_INVALID;
		pconstParam.type = pConst.base.type;
		pconstParam.stage = MatParamStage::Fragment;
		pconstParam.offset = pConst.base.offset;
		pconstParam.size = pConst.base.size;
		pconstParam.nameIndex = pConst.base.nameIndex;
		for (auto& member : pConst.members) {
			MatParam pconstParamMember{};
			pconstParamMember.parentNameIndex = pconstParam.nameIndex;
			pconstParamMember.type = member.type;
			pconstParamMember.stage = MatParamStage::Fragment;
			pconstParamMember.offset = member.offset;
			pconstParamMember.nameIndex = member.nameIndex;
			pconstParam.members.push_back(pconstParamMember);
		}
		psParams.push_back(pconstParam);
	}


	std::vector<MatParam> fromPs;
	for (auto& psParam : psParams) {

		bool foundDuplicate = false;
		for (auto& vsParam : vsParams) {
			if (vsParam.sharesDataExceptStage(psParam)) {
				this_.setParamStageRecursive(vsParam, MatParamStage::Both);
				foundDuplicate = true;
				break;
			}
		}

		if (!foundDuplicate)
			fromPs.push_back(psParam);
	}
	for (auto& vsParam : vsParams)
		this_.params.push_back(vsParam);
	for (auto& psParam : fromPs)
		this_.params.push_back(psParam);

	//descriptorSetKeys.clear();
	for (size_t i = 0; i < this_.params.size(); ++i ) {
		auto& param = this_.params[i];
		this_.paramsMap.insert({ param.name(), i });

		// Skip push constants
		if (param.type != TypeBase::PushConstStruct) {
			auto& descSetKey = this_.descriptorSetKeys[param.setIndex];
			descSetKey.setIndex = param.setIndex;
			descSetKey.bindings.push_back(param.bindingIndex);
			descSetKey.types.push_back(param.descriptorType);
			descSetKey.nameIndices.push_back(param.nameIndex);
			descSetKey.counts.push_back(param.count);
			descSetKey.stageFlags.push_back(MatParamStageToVkShaderStageFlagBits(param.stage));
		}

		switch (param.type)
		{
			case TypeBase::PushConstStruct:
			{
				this_.pushShaderFlags = MatParamStageToVkShaderStageFlagBits(param.stage);
				this_.pushConstantOffset = param.offset;
				this_.pushConstantSize = param.size;
			} break;

			case TypeBase::Sampler:
			{
				this_.samplerStates.insert({ {param.bindingIndex, param.setIndex}, SamplerState{} });
			} break;

			case TypeBase::StructBuffer:
			case TypeBase::CBuffer:
			{
				if (param.setIndex >= VULKAN_GLOBAL_DESCRIPTOR_SETS) {
					auto thisIndex = this_.buffers.size();
					this_.buffers.emplace_back(MaterialBuffer{param.members[0]});
					this_.bufferNameIndexMap.insert({param.nameIndex, thisIndex});
				}
			} break;

		}
	}

	for (auto& [set, key] : this_.descriptorSetKeys) {
		key.sortByBinding();
	}
	return this_;
}

bool Material::operator==(const Material& other) {
	if (other.pShaderName != pShaderName)
		return false;
	if (other.vShaderName != vShaderName)
		return false;
	if (other.paramsMap.size() != paramsMap.size())
		return false;
	for (size_t i = 0; i < paramsMap.size(); ++i) {
		if (params[i] != other.params[i])
			return false;
	}

	return true;
}

void Material::debugPrintMaterialInfo() {
	size_t depth = 0;
	auto* vs = RenderManager::getInstance()->getShader(vShaderName);
	auto* ps = RenderManager::getInstance()->getShader(pShaderName);


	std::cout << "* Material Info - " << name();
	std::cout << std::format("\n\nVertex Shader - {}, Entry: {}\n  Input:", vs->name(), vs->entryPoint);
	auto& vsAttribs = RenderManager::getInstance()->getShader(vShaderName)->input.attributes;
	for (auto& attrib : vsAttribs) {
		std::cout << "\n\t" << std::format("[{}] ", attrib.location == 255 ? "" : std::to_string(attrib.location)) <<
			TypeBaseToString(attrib.type) << " : " << attrib.semantic();
	}
	std::cout << std::format("\n\nPixel Shader - {}, Entry: {}\n  Input:", ps->name(), ps->entryPoint); //"\n\nPS Input:";
	auto& attribs = RenderManager::getInstance()->getShader(pShaderName)->input.attributes;
	for (auto& attrib : attribs) {
		std::cout << "\n\t" << std::format("[{}] ", attrib.location == 255 ? "" : std::to_string(attrib.location)) <<
			TypeBaseToString(attrib.type) << " : " << attrib.semantic();
	}
	std::cout << "\n  Output:";
	auto& outAttribs = RenderManager::getInstance()->getShader(pShaderName)->output.attributes;
	for (auto& attrib : outAttribs) {
		std::cout << "\n\t" << std::format("[{}] ", attrib.location == 255 ? "" : std::to_string(attrib.location)) <<
			TypeBaseToString(attrib.type) << " : " << attrib.semantic();
	}


	std::cout << "\n\n * Parameters:\n";


	debugPrintParameters(++depth, params);
}

void Material::debugPrintParameters(size_t& depth, std::vector<MatParam>& params) {
	std::string pad = "";
	
	for (size_t i = 0; i < depth; ++i)
		pad.append("\t");

	for (auto& param : params) {
		std::string shaders = "    \t";
		if (depth == 1) {
			switch (param.stage) {
				case MatParamStage::Both:
					shaders = "VS,PS\t"; break;
				case MatParamStage::Fragment:
					shaders = "   PS\t"; break;
				case MatParamStage::Vertex:
					shaders = "VS  \t"; break;
			}
		}
		std::string arrayType = param.arrayType == MatParamArrayType::Dynamic 
			? " (bindless)" 
			: (param.arrayType == MatParamArrayType::Static && !(param.type == TypeBase::Struct || param.type == TypeBase::PushConstStruct))
				? std::format("[{}]", param.count)
				: "";
		std::string binding = depth == 1 ? std::format("[{}, {}]", param.bindingIndex, param.setIndex) : "";
		binding = param.type == TypeBase::PushConstStruct ? "" : binding;
		std::cout << shaders << binding << pad << TypeBaseToString(param.type) << " " << param.name() << arrayType << "\n";
		debugPrintParameters(++depth, param.members);
	}
	depth--;
}


std::string& Material::name() {
	return RenderManager::getInstance()->getMaterialName(*this);
}
std::string& Material::name() const {
	return RenderManager::getInstance()->getMaterialName(*this);
}
MaterialBuffer* Material::getBuffer(const std::string& bufferName) {
	auto nameIndex = RenderManager::getInstance()->getParamNameIndex(bufferName);
	if (nameIndex != SIZE_T_INVALID) {
		auto it = bufferNameIndexMap.find(nameIndex);
		if (it != bufferNameIndexMap.end())
			return &buffers[it->second];
	}
	return nullptr;
}











MaterialBuffer* MaterialInstance::getBuffer(const std::string& bufferName) {
	return base->getBuffer(bufferName);
}
void MaterialInstance::setParameter(const std::string& bufferName, const std::string& paramName, void* value) {
	auto buffer = base->getBuffer(bufferName);
	if (buffer != nullptr) {
		auto result = buffer->setParameter(paramName, value, m_instanceIndex);
		if (!result)
			LOGLINE(LogType::Warning, LogMod::Rendering,
					std::format("setParameter: parameter '{}' not found.", paramName));
	} else {
		LOGLINE(LogType::Warning, LogMod::Rendering,
				std::format("setParameter: buffer '{}' not found.", bufferName));
	}
}