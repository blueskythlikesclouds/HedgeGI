#include "ShaderLibrary.h"
#include "Shader.h"

#include <fstream>

ShaderLibrary::ShaderLibrary(const std::string& directoryPath) : directoryPath(directoryPath)
{
}

const Shader& ShaderLibrary::create(const std::string& name)
{
    const auto& pair = shaders.find(name);

    if (pair != shaders.end())
        return *pair->second;

    std::ifstream vertexShaderStream(directoryPath + "/" + name + ".vert.glsl");
    std::ifstream fragmentShaderStream(directoryPath + "/" + name + ".frag.glsl");

    std::stringstream vertexShaderStringStream;
    std::stringstream fragmentShaderStringStream;

    vertexShaderStringStream << vertexShaderStream.rdbuf();
    fragmentShaderStringStream << fragmentShaderStream.rdbuf();

    const std::string vertexShader = vertexShaderStringStream.str();
    const std::string fragmentShader = fragmentShaderStringStream.str();

    return *(shaders[name] = std::move(Shader::create(vertexShader.c_str(), (GLint)vertexShader.size(), fragmentShader.c_str(), (GLint)fragmentShader.size())));
}
