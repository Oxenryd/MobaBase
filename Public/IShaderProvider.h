#ifndef ISHADERPROVIDER_H
#define ISHADERPROVIDER_H

#include <string>
#include <cstdint>
#include "Shader.h"

class IShaderProvider
{
public:
	virtual Shader* const getShader(const std::string& name) const = 0;
	virtual const std::string& getParamName(size_t nameIndex) const = 0;
	virtual ~IShaderProvider() = default;
};

#endif