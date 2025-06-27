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
		j["params"].push_back(p.toJson());

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
		params.push_back(std::move(param));
	}
	ShaderManager::getInstance()->registerMaterial(newName, *this);
}

void Material::addShaderParams(
	std::vector<MatParam>& params,
	std::vector<ShaderParameter>& s,
	MatParamStage stage)
{
	for (ShaderParameter& param : s) {
		MatParam matParam;
		matParam.state.type = Shader::parseReflectedTypeDesc(&param.spvTypeDesc);
		matParam.state.stage = static_cast<uint32_t>(stage);
		matParam.state.count = param.count;
		matParam.bindingIndex = param.binding;
		matParam.descriptorType = param.descriptorType;
		matParam.offset = param.offset;
		matParam.value.asPtr = nullptr;
		addShaderParams(params, param.members, stage);
		params.push_back(std::move(matParam));
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

	addShaderParams(params, vertex.parameters, MatParamStage::Vertex);
	addShaderParams(params, fragment.parameters, MatParamStage::Fragment);

	ShaderManager::getInstance()->registerMaterial(newName, *this);
}

std::string& Material::name() {
	return ShaderManager::getInstance()->getMaterialName(*this);
}