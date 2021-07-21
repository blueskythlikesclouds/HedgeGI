#pragma once

class Shader
{
    static GLuint create(GLenum type, const GLchar* string, GLint length);

public:
    static std::unique_ptr<Shader> create(const GLchar* vertexShader, GLint vertexShaderLength, const GLchar* fragmentShader, GLint fragmentShaderLength);

    const GLuint id;

    Shader(GLuint id);
    ~Shader();

    void use() const;   
};
