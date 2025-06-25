#ifndef FS_H
#define FS_H

#include <filesystem>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <cstdio>

#include "Shader.h"
#include "ErrorCodes.hpp"

class FS
{
public:
	static std::vector<std::filesystem::path> getAllFilesWithExtension(
		const std::filesystem::path& directory, const std::wstring& extension);

	static std::filesystem::path getExecutableDir();

	static Shader::Type parseShaderTypeFromFile(const std::filesystem::path& filePath);

	static ErrorCode execAndCapture(const std::string& command, std::string& outStr);

	static std::string parseShaderNameFromFile(const std::filesystem::path& filePath);
};
#endif