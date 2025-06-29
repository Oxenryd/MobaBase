#include "Material.hpp"
#include "Shader.h"
#include "ShaderManager.h"

json Material::toJson() {
	json j;
	j["name"] = name();
	j["vShaderName"] = vShaderName;
	j["pShaderName"] = pShaderName;

	j["params"] = json::array();
	for (auto& p : params)
		j["params"].push_back(p.second.toJson());

	return j;
}

void Material::fromJson(const json& j) {
	auto newName = j.at("name").get<std::string>();
	vShaderName = j.at("vShaderName").get<std::string>();
	pShaderName = j.at("pShaderName").get<std::string>();

	params.clear();
	for (const auto& jp : j.at("params")) {
		MatParam param;
		param.fromJson(jp);
		params.insert({param.name, std::move(param) });
	}
	ShaderManager::getInstance()->registerMaterial(newName, *this);
}

void Material::addShaderParams(
	std::map<std::string, MatParam>& params,
	std::vector<ShaderBinding>& s,
	MatParamStage stage, const std::string& parentName)
{
	for (ShaderBinding& param : s) {
		MatParam matParam;
		matParam.parentName = parentName;
		matParam.name = param.name;
		matParam.state.type = Shader::parseReflectedTypeDesc(&param.spvTypeDesc);
		matParam.state.stage = static_cast<uint32_t>(stage);
		matParam.state.count = param.count;
		matParam.bindingIndex = param.binding;
		matParam.descriptorType = param.descriptorType;
		matParam.offset = param.offset;
		matParam.value.asPtr = nullptr;
		addShaderParams(matParam.members, param.members, stage, matParam.name);
		params.insert({ matParam.name, std::move(matParam) });
	}
}

void Material::initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment) {
	vShaderName = vertex.name();
	pShaderName = fragment.name();

	params.clear();

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

	std::map<std::string, MatParam> vsParam;
	std::map<std::string, MatParam> psParam;
	addShaderParams(vsParam, vertex.parameters, MatParamStage::Vertex, "");
	addShaderParams(psParam, fragment.parameters, MatParamStage::Fragment, "");

	for (auto& vsMatParam : vsParam) {
		auto it = psParam.find(vsMatParam.first);
		if (it != psParam.end()) {
			if (it->second.parentName == vsMatParam.second.parentName) {
				vsMatParam.second.state.stage = static_cast<uint32_t>(MatParamStage::Both);
				it->second.state.stage = static_cast<uint32_t>(MatParamStage::Both);
				psParam.erase(it->first);
			}
		}
	}
	for (auto& vsMatParam : vsParam) 
		params.insert({vsMatParam.first, vsMatParam.second});
	for (auto& psMatParam : psParam)
		params.insert({ psMatParam.first, psMatParam.second });

	ShaderManager::getInstance()->registerMaterial(newName, *this);
}


std::string& Material::name() {
	return ShaderManager::getInstance()->getMaterialName(*this);
}