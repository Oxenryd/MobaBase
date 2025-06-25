#include "Material.hpp"
#include "Shader.h"
#include "ShaderManager.h"

json Material::toJson() {
	json j;
	j["name"] = name();
	j["vShaderName"] = vShaderName;
	j["pShaderName"] = pShaderName;

	j["params"] = json::array();
	for (const auto& p : params)
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

void Material::initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment) {
	vShaderName = vertex.name();
	pShaderName = fragment.name();

	params.clear();

	// Merge descriptors from both shaders
	auto addShaderParams = [&](const Shader& s, MatParamStage stage) {
		for (const auto& param : s.parameters) {
			MatParam matParam;
			matParam.state.type = MatParamUtils::deduceMatParamType(param.descriptorType);
			matParam.state.stage = stage;
			matParam.state.count = param.count;
			matParam.bindingIndex = param.binding;
			matParam.descriptorType = param.descriptorType;
			matParam.offset = param.offset;
			matParam.value.asPtr = nullptr;
			params.push_back(std::move(matParam));
		}
		};

	addShaderParams(vertex, MatParamStage::Vertex);
	addShaderParams(fragment, MatParamStage::Fragment);

	ShaderManager::getInstance()->registerMaterial(newName, *this);
}

std::string& Material::name() {
	return ShaderManager::getInstance()->getMaterialName(*this);
}