#pragma once

class Shader;

class ShaderLibrary
{
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;

public:
    const std::string directoryPath;

    ShaderLibrary(const std::string& directoryPath);

    const Shader& create(const std::string& name);
};
