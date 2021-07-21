#include "Shader.h"

GLuint Shader::create(GLenum type, const GLchar* string, GLint length)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &string, &length);
    glCompileShader(id);

    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(id, sizeof(infoLog), nullptr, infoLog);

        printf(infoLog);
        getchar();
    }

    return id;
}

std::unique_ptr<Shader> Shader::create(const GLchar* vertexShader, GLint vertexShaderLength, const GLchar* fragmentShader, GLint fragmentShaderLength)
{
    GLuint vertexShaderId = create(GL_VERTEX_SHADER, vertexShader, vertexShaderLength);
    GLuint fragmentShaderId = create(GL_FRAGMENT_SHADER, fragmentShader, fragmentShaderLength);

    GLuint id = glCreateProgram();
    glAttachShader(id, vertexShaderId);
    glAttachShader(id, fragmentShaderId);
    glLinkProgram(id);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return std::make_unique<Shader>(id);
}

Shader::Shader(GLuint id) : id(id)
{
}

Shader::~Shader()
{
    glDeleteProgram(id);
}

void Shader::use() const
{
    glUseProgram(id);
}
