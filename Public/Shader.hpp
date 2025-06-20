#ifndef SHADER_HPP
#define SHADER_HPP

#include <filesystem>
#include <cstdint>
#include <chrono>

class Shader
{
private:
    
    size_t m_arrayIndex = static_cast<size_t>(-1);

public:
    enum class Type : uint8_t
    {
        Invalid,
        Vertex,
        Fragment,
        Compute
    };

    Type type;
    std::filesystem::path sourcePath;
    std::filesystem::file_time_type lastSourceChangedTime;
    std::vector<char> bytecode;


    Shader(Type type, const std::filesystem::path& path)
        : type(type), sourcePath(path) {

        lastSourceChangedTime = std::filesystem::last_write_time(sourcePath);
    }

    friend void SetShaderArrayIndex(Shader& shader, size_t index);
};

inline void SetShaderArrayIndex(Shader& shader, size_t index) {
    shader.m_arrayIndex = index;
}

#endif // ! SHADER_HPP