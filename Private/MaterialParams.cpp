#include "MaterialParams.h"
#include "ShaderManager.h"

std::string& MatParam::name() {
	return ShaderManager::getInstance()->paramNames()[nameIndex];
}
std::string& MatParam::name() const {
	return ShaderManager::getInstance()->paramNames()[nameIndex];
}

std::string& MatParam::parentName() {
	return ShaderManager::getInstance()->paramNames()[parentNameIndex];
}

std::string& MatParam::parentName() const {
	return ShaderManager::getInstance()->paramNames()[parentNameIndex];
}
