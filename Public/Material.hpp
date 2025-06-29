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
	void fromJson(const json& j);
	void addShaderParams(std::map<std::string, MatParam>& params, std::vector<ShaderBinding>& s, MatParamStage stage, const std::string& parentName);
public:
	std::map<std::string, MatParam> params;
	std::string vShaderName;
	std::string pShaderName;

	Material() = default;
	Material(const json& j) { fromJson(j); }
	Material(const std::string& name, Shader& vertex, Shader& fragment) { initFromShaders(name, vertex, fragment); }

	std::string& name();

	json toJson();
	

	void initFromShaders(const std::string& newName, Shader& vertex, Shader& fragment);
};

#endif