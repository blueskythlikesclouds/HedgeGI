#include "ShaderProgram.h"
#include "resource.h"

#define SHADER_VER "#version 330"
#define SHADER_VER_LEN strlen(SHADER_VER)

#define MAX_SHADER_INFOS 3

struct ShaderInfo
{
    GLenum type;
    size_t resource;
    const char* defines;
};

struct ShaderProgramInfo
{
    const char* name;
    ShaderInfo shaders[MAX_SHADER_INFOS];
};

const ShaderProgramInfo shaderInfos[] =
{
    {
        "CopyTexture",
        {
            {GL_VERTEX_SHADER, ResVertexShader_CopyTexture},
            {GL_FRAGMENT_SHADER, ResFragmentShader_CopyTexture}
        }
    },
    {
        "ToneMap",
        {
            {GL_VERTEX_SHADER, ResVertexShader_ToneMap},
            {GL_FRAGMENT_SHADER, ResFragmentShader_ToneMap}
        }
    },
    {
        "Im3d_Points",
        {
            {GL_VERTEX_SHADER, ResShader_Im3d, "POINTS\0VERTEX_SHADER\0"},
            {GL_FRAGMENT_SHADER, ResShader_Im3d, "POINTS\0FRAGMENT_SHADER\0"}
        }
    },
    {
        "Im3d_Lines",
        {
            {GL_VERTEX_SHADER, ResShader_Im3d, "LINES\0VERTEX_SHADER\0"},
            {GL_GEOMETRY_SHADER, ResShader_Im3d, "LINES\0GEOMETRY_SHADER\0"},
            {GL_FRAGMENT_SHADER, ResShader_Im3d, "LINES\0FRAGMENT_SHADER\0"}
        }
    },
    {
        "Im3d_Triangles",
        {
            {GL_VERTEX_SHADER, ResShader_Im3d, "TRIANGLES\0VERTEX_SHADER\0"},
            {GL_FRAGMENT_SHADER, ResShader_Im3d, "TRIANGLES\0FRAGMENT_SHADER\0"}
        }
    },
    {
        "Billboard",
        {
            {GL_VERTEX_SHADER, ResVertexShader_Billboard},
            {GL_FRAGMENT_SHADER, ResFragmentShader_Billboard}
        }
    },
};

std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> ShaderProgram::shaders;

GLuint ShaderProgram::create(GLenum type, const char* defines, const char* shader, size_t shaderSize)
{
    std::string str;
    GLint size = (GLint)shaderSize;

    if (defines)
    {
        str = SHADER_VER "\n";

        size_t defineSize;
        while ((defineSize = strlen(defines)) != 0)
        {
            str += "#define ";
            str += defines;
            str += "\n";
            defines += defineSize + 1;
        }

        for (int i = 0; i < shaderSize - SHADER_VER_LEN; i++)
        {
            const char* current = shader + i;

            if (strncmp(current, SHADER_VER, SHADER_VER_LEN) != 0)
                continue;

            current += SHADER_VER_LEN;

            shaderSize = shader + shaderSize - current;
            shader = current;

            break;
        }

        str.append(shader, shaderSize);

        shader = str.c_str();
        size = (GLint)str.size();
    }

    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &shader, &size);
    glCompileShader(id);

    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(id, sizeof(infoLog), nullptr, infoLog);

        MessageBoxA(nullptr, infoLog, "HedgeGI", MB_ICONERROR);
    }

    return id;
}

void ShaderProgram::registerUniforms()
{
    GLint count;
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);

    for (GLint i = 0; i < count; i++)
    {
        GLsizei length;
        GLint size;
        GLenum type;
        GLchar name[0x100] {};

        glGetActiveUniform(id, i, sizeof(name), &length, &size, &type, name);

        uniforms[name] = glGetUniformLocation(id, name);
    }
}

const ShaderProgram& ShaderProgram::get(const char* name)
{
    const auto& pair = shaders.find(name);

    if (pair != shaders.end())
        return *pair->second;

    const ShaderProgramInfo* info = nullptr;

    for (size_t i = 0; i < _countof(shaderInfos); i++)
    {
        if (shaderInfos[i].name != name && strcmp(shaderInfos[i].name, name) != 0)
            continue;

        info = &shaderInfos[i];
        break;
    }

    const GLuint id = glCreateProgram();
    GLuint shaderArr[MAX_SHADER_INFOS];

    for (size_t i = 0; i < MAX_SHADER_INFOS; i++)
    {
        if (info->shaders[i].type == GL_NONE)
        {
            shaderArr[i] = GL_NONE;
            continue;
        }

        const HRSRC hResInfo = FindResource(nullptr, MAKEINTRESOURCE(info->shaders[i].resource), TEXT("TEXT"));
        const HGLOBAL hResData = LoadResource(nullptr, hResInfo);

        glAttachShader(id, shaderArr[i] = 
            create(info->shaders[i].type, info->shaders[i].defines, 
                (const GLchar*)LockResource(hResData), SizeofResource(nullptr, hResInfo)));

        FreeResource(hResData);
    }

    glLinkProgram(id);

    for (const auto shader : shaderArr)
    {
        if (shader != GL_NONE)
            glDeleteShader(shader);
    }

    return *(shaders[name] = std::make_unique<ShaderProgram>(id));
}

ShaderProgram::ShaderProgram(GLuint id) : id(id)
{
    registerUniforms();
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(id);
}

void ShaderProgram::use() const
{
    glUseProgram(id);
}

GLint ShaderProgram::getUniformLocation(const char* name) const
{
    auto pair = uniforms.find(name);

    if (pair != uniforms.end())
        return pair->second;

    return glGetUniformLocation(id, name);
}

void ShaderProgram::set(const char* name, const GLint value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void ShaderProgram::set(const char* name, const GLfloat value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void ShaderProgram::set(const char* name, const bool value) const
{
    glUniform1i(getUniformLocation(name), (GLint)value);
}

void ShaderProgram::set(const char* name, const Vector2& value) const
{
    glUniform2fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const char* name, const Vector3& value) const
{
    glUniform3fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const char* name, const Vector4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const char* name, const Color4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const char* name, const Matrix4& value) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, false, value.data());
}
