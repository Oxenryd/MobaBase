#include "Material.hpp"
#include "Shader.h"
#include "ShaderManager.h"
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
		MatParam matParam;
		//matParam.parentName = parentName;
		//matParam.name = param.name;
		matParam.type = Shader::parseReflectedTypeDesc(&param.spvTypeDesc, &param.descriptorType);
		matParam.stage = stage;
		matParam.count = param.count;
		matParam.bindingIndex = param.binding;
		matParam.setIndex = param.set;
		//matParam.descriptorType = param.descriptorType;
		matParam.offset = param.offset;
		//matParam.value.asPtr = nullptr;

		std::string name;
		auto nameIndex = ShaderManager::getInstance()->getParamNameIndex(param.name);
		if (nameIndex == SIZE_T_INVALID) {
			name = param.name;
			matParam.nameIndex = ShaderManager::getInstance()->registerParamName(name);
		} else {
			matParam.nameIndex = nameIndex;
			name = ShaderManager::getInstance()->getParamName(nameIndex);
		}


		if (!parentName.empty()) {
			auto parentNameIndex = ShaderManager::getInstance()->getParamNameIndex(parentName);
			if (parentNameIndex == SIZE_T_INVALID) {
				matParam.parentNameIndex = ShaderManager::getInstance()->registerParamName(parentName);
			} else {
				matParam.parentNameIndex = parentNameIndex;
			}
		} else
			matParam.parentNameIndex = UINT32_INVALID;


		addShaderParams(matParam.members, param.members, stage, name);
		
		//paramsMap.insert({ matParam.name, params.size()});
		params.push_back(matParam);
	}
}

void Material::initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment) {
	vShaderName = vertex.name();
	pShaderName = fragment.name();

	paramsMap.clear();

	// Merge descriptors from both shaders
	//void* addShaderParams = [](std::vector<MatParam> params, const std::vector<ShaderParameter>& s, MatParamStage stage) {
	//	for (const auto& param : s) {
	//		MatParam matParam;
	//		matParam.state.type = Shader::parseReflectedTypeDesc(&param.spvTypeDesc);
	//		matParam.state.stage = static_cast<uint32_t>(stage);
	//		matParam.state.count = param.count;
	//		matParam.bindingIndex = param.binding;
	//		matParam.descriptorType = param.descriptorType;
	//		matParam.offset = param.offset;
	//		matParam.value.asPtr = nullptr;
	//		addShaderParams(param.members, stage);
	//		params.push_back(std::move(matParam));
	//	}
	//};

	//std::map<std::string, size_t> vsParamsMap;
	std::vector<MatParam> vsParams;
	//std::map<std::string, size_t> psParamsMap;
	std::vector<MatParam> psParams;
	addShaderParams(vsParams, vertex.parameters, MatParamStage::Vertex, "");
	addShaderParams(psParams, fragment.parameters, MatParamStage::Fragment, "");

	//for (auto& vsMatParam : vsParamsMap) {
	//	auto it = psParamsMap.find(vsMatParam.first);
	//	if (it != psParamsMap.end()) {
	//		if (it->second.parentName == vsMatParam.second.parentName) {
	//			vsMatParam.second.stage = MatParamStage::Both;
	//			it->second.state.stage = static_cast<uint32_t>(MatParamStage::Both);
	//			psParamsMap.erase(it->first);
	//		}
	//	}
	//}
	//for (auto& vsMatParam : vsParamsMap) 
	//	paramsMap.insert({vsMatParam.first, vsMatParam.second});
	//for (auto& psMatParam : psParamsMap)
	//	paramsMap.insert({ psMatParam.first, psMatParam.second });

	
	std::vector<MatParam> fromPs;
	for (auto& psParam : psParams) {

		bool foundDuplicate = false;
		for (auto& vsParam : vsParams) {
			if (vsParam.sharesDataExceptStage(psParam)) {
				setParamStageRecursive(vsParam, MatParamStage::Both);
				foundDuplicate = true;
				break;
			}
		}

		if (!foundDuplicate)
			fromPs.push_back(psParam);
	}
	for (auto& vsParam : vsParams)
		params.push_back(vsParam);
	for (auto& psParam : fromPs)
		params.push_back(psParam);

	ShaderManager::getInstance()->registerMaterial(newName, *this);
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
	std::cout << "Material Info * - " << name() << ":\n";
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
					shaders = "PS  \t"; break;
				case MatParamStage::Vertex:
					shaders = "VS  \t"; break;
			}
		}

		std::string binding = depth == 1 ? std::format("[{}, {}]", param.bindingIndex, param.setIndex) : "";
		std::cout << shaders << binding << pad << TypeBaseToString(param.type) << " " << param.name() << "\n";
		debugPrintParameters(++depth, param.members);
	}
	depth--;
}


std::string& Material::name() {
	return ShaderManager::getInstance()->getMaterialName(*this);
}