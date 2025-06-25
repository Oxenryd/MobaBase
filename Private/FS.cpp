#include "FS.h"
#ifdef BUILD_WIN
	#include <windows.h>
#endif
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>



std::vector<std::filesystem::path> FS::getAllFilesWithExtension(
	const std::filesystem::path& directory, const std::wstring& extension) {
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		auto regularFile = entry.is_regular_file();
		auto fExtension = entry.path().extension();
		if (regularFile && fExtension == extension) {
			files.push_back(entry.path());
		}
	}
	return files;
}

std::filesystem::path FS::getExecutableDir() {
#ifdef BUILD_WIN
	char buffer[1024];
	GetModuleFileNameA(NULL, buffer, sizeof(buffer));
	std::filesystem::path exePath(buffer);
	return exePath.parent_path();
#endif
}

Shader::Type FS::parseShaderTypeFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("#pragma") != std::string::npos) {
            if (line.find(SHADER_TYPE_VERTEX) != std::string::npos)
                return Shader::Type::Vertex;
            if (line.find(SHADER_TYPE_FRAGMENT) != std::string::npos)
                return Shader::Type::Fragment;
            if (line.find(SHADER_TYPE_COMPUTE) != std::string::npos)
                return Shader::Type::Compute;
        }
    }
	return Shader::Type::Invalid;
}

std::string FS::parseShaderNameFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("#pragma") != std::string::npos) {
            std::size_t namePos = line.find(SHADER_NAME);
            if (namePos != std::string::npos) {
                std::string name = line.substr(namePos + std::strlen(SHADER_NAME));
                name.erase(0, name.find_first_not_of(" \t\r\n"));
                name.erase(name.find_last_not_of(" \t\r\n") + 1);
                return name;
            }
        }
    }
    return "";
}

ErrorCode FS::execAndCapture(const std::string& command, std::string& outStr) {
    std::array<char, 1024> buffer;
    outStr.clear();

    // Use "r" for reading stdout
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    if (!pipe)
        return ErrorCode::COMMAND_FAILED_EXECUTION;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        outStr += buffer.data();

    int returnCode = _pclose(pipe.release());
    if (returnCode == 0)
        return ErrorCode::OK;
    else {
        outStr = "execAndCapture() exited with Code: " + std::to_string(returnCode) + ". ";
        return static_cast<ErrorCode>(returnCode);
    }
}
