#include "FileSys.h"

#include <unistd.h>
#ifdef BUILD_WIN
	#include <windows.h>
#endif


#include <string>
#include <vector>
#include <fstream>
#include <filesystem>



std::vector<std::filesystem::path> FileSys::getAllFilesWithExtension(
	const std::filesystem::path& directory, const std::wstring& extension) {
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		const auto regularFile = entry.is_regular_file();
		auto fExtension = entry.path().extension();
		if (regularFile && fExtension == extension) {
			files.push_back(entry.path());
		}
	}
	return files;
}

std::filesystem::path FileSys::getExecutableDir() {

#ifdef BUILD_WIN
	char buffer[1024];
	GetModuleFileNameA(NULL, buffer, sizeof(buffer));
    const std::filesystem::path exePath(buffer);
    return exePath.parent_path();
#else
    char buffer[PATH_MAX]{};
    const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        throw std::runtime_error("readlink(/proc/self/exe) failed.");
    }
    buffer[len] = '\0';
    const std::filesystem::path exePath(buffer);
    return exePath.parent_path();
#endif
}

Shader::Type FileSys::parseShaderTypeFromFile(const std::filesystem::path& filePath) {
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

std::string FileSys::parseShaderNameFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("#pragma") != std::string::npos) {
            const std::size_t namePos = line.find(SHADER_NAME);
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

ErrorCode FileSys::execAndCapture(const std::string& command, std::string& outStr) {

#ifdef BUILD_WIN
    std::array<char, 1024> buffer{};
    outStr.clear();

    // Use "r" for reading stdout
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    if (!pipe)
        return ErrorCode::COMMAND_FAILED_EXECUTION;

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
        outStr += buffer.data();

    int returnCode = _pclose(pipe.release());
    if (returnCode == 0)
        return ErrorCode::OK;
    else {
        outStr = "execAndCapture() exited with Code: " + std::to_string(returnCode) + ". ";
        return static_cast<ErrorCode>(returnCode);
    }
#else

    std::array<char, 4096> buffer{};
    outStr.clear();

    // If you also want stderr on Linux:
    // std::string cmd = command + " 2>&1";
    const std::string& cmd = command;

    FILE* raw = popen(cmd.c_str(), "r");
    if (!raw)
        return ErrorCode::COMMAND_FAILED_EXECUTION;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(raw, pclose);

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        outStr += buffer.data();

    // IMPORTANT: call pclose exactly once
    const int status = pclose(pipe.release());

    // Convert POSIX wait status to exit code (shell command exit code)
    if (status != -1) {
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode == 0)
                return ErrorCode::OK;

            outStr = "execAndCapture() exited with Code: " + std::to_string(exitCode) + ". " + outStr;
            return static_cast<ErrorCode>(exitCode);
        }

        // Process ended abnormally (signal, etc.)
        outStr = "execAndCapture() terminated abnormally. " + outStr;
        return ErrorCode::COMMAND_FAILED_EXECUTION;
    }

    outStr = "execAndCapture() failed to close pipe (pclose returned -1). " + outStr;
    return ErrorCode::COMMAND_FAILED_EXECUTION;

#endif
}
