#ifndef SHADER_HPP
#define SHADER_HPP

#include <filesystem>

class Shader
{
public:
    enum class ShaderType : uint8_t
    {
        Vertex,
        Fragment
    };

    ShaderType type;
    std::filesystem::path sourcePath;
    std::filesystem::file_time_type lastWriteTime;
    std::vector<uint8_t> bytecode;

    Shader(ShaderType type, const std::filesystem::path& path)
        : type(type), sourcePath(path) {
        lastWriteTime = std::filesystem::last_write_time(sourcePath);
    }
};

#endif // ! SHADER_HPP