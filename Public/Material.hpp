#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <map>

#include "ErrorCodes.hpp"
#include "MaterialParams.h"

#ifdef USE_VULKAN
	#ifndef VULKAN_CORE_H_
		#include <vulkan/vulkan_core.h>
	#endif
#endif

#include "Texture.hpp"

class Shader;
class ShaderManager;
struct ShaderBinding;
struct PsoDesc;

class Material
{
private:
	friend ShaderManager;
	size_t m_arrayIndex;
	//void fromJson(const json& j);
	void addShaderParams(
		std::vector<MatParam>& params,
		std::vector<ShaderBinding>& s,
		MatParamStage stage,
		const std::string& parentName);

	void setParamStageRecursive(MatParam& param, MatParamStage stage) {
		param.stage = stage;
		for (auto& p : param.members) {
			setParamStageRecursive(p, stage);
		}
	}

public:
	std::map<std::string, size_t> paramsMap;
	std::vector<MatParam> params;
	std::string vShaderName;
	std::string pShaderName;

	Material() = default;
	//Material(const json& j) { fromJson(j); }
	Material(const std::string& name, Shader& vertex, Shader& fragment) { 
		initFromShaders(name, vertex, fragment);
	}

	std::string& name();

	//json toJson();
	

	void initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);

	bool operator==(const Material& other);
	bool operator!=(const Material& other) { return !(*this == other); }

	void debugPrintMaterialInfo();
	void debugPrintParameters(size_t& depth, std::vector<MatParam>& params);
};

#endif