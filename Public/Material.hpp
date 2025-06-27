#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "MaterialParams.h"

#ifdef USE_VULKAN
	#ifndef VULKAN_CORE_H_
		#include <vulkan/vulkan_core.h>
	#endif
#endif

#include "Texture.hpp"

class Shader;
class ShaderManager;
struct ShaderParameter;
class Material
{
private:
	friend ShaderManager;
	size_t m_arrayIndex;
	void fromJson(const json& j);
	void addShaderParams(std::vector<MatParam>& params, std::vector<ShaderParameter>& s, MatParamStage stage);
public:
	std::vector<MatParam> params;
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