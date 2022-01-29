#include "ShaderProgram.h"
#include "resource.h"

const struct ShaderDescriptor
{
    const char* const name;
    const size_t vertexShader;
    const size_t fragmentShader;
} shaderDescriptors[] =
{
    { "CopyTexture", ResVertexShader_CopyTexture, ResFragmentShader_CopyTexture },
    { "ToneMap", ResVertexShader_ToneMap, ResFragmentShader_ToneMap },
    { "Im3d", ResVertexShader_Im3d, ResFragmentShader_Im3d },
    { "Billboard", ResVertexShader_Billboard, ResFragmentShader_Billboard }
};

std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> ShaderProgram::shaders;

GLuint ShaderProgram::create(GLenum type, const GLchar* string, GLint length)
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

std::unique_ptr<ShaderProgram> ShaderProgram::create(const GLchar* vertexShader, GLint vertexShaderLength, const GLchar* fragmentShader, GLint fragmentShaderLength)
{
    GLuint vertexShaderId = create(GL_VERTEX_SHADER, vertexShader, vertexShaderLength);
    GLuint fragmentShaderId = create(GL_FRAGMENT_SHADER, fragmentShader, fragmentShaderLength);

    GLuint id = glCreateProgram();
    glAttachShader(id, vertexShaderId);
    glAttachShader(id, fragmentShaderId);
    glLinkProgram(id);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return std::make_unique<ShaderProgram>(id);
}

const ShaderProgram& ShaderProgram::get(const char* name)
{
    const auto& pair = shaders.find(name);

    if (pair != shaders.end())
        return *pair->second;

    const ShaderDescriptor* shaderDescriptor = nullptr;
    for (size_t i = 0; i < _countof(shaderDescriptors); i++)
    {
        if (strcmp(shaderDescriptors[i].name, name) != 0)
            continue;

        shaderDescriptor = &shaderDescriptors[i];
        break;
    }

    const HRSRC hVertexShaderResource = FindResource(nullptr, MAKEINTRESOURCE(shaderDescriptor->vertexShader), TEXT("TEXT"));
    const HRSRC hFragmentShaderResource = FindResource(nullptr, MAKEINTRESOURCE(shaderDescriptor->fragmentShader), TEXT("TEXT"));

    const HGLOBAL hVertexShaderGlobal = LoadResource(nullptr, hVertexShaderResource);
    const HGLOBAL hFragmentShaderGlobal = LoadResource(nullptr, hFragmentShaderResource);

    const size_t vertexShaderSize = SizeofResource(nullptr, hVertexShaderResource);
    const size_t fragmentShaderSize = SizeofResource(nullptr, hFragmentShaderResource);

    const char* vertexShader = (const char*)LockResource(hVertexShaderGlobal);
    const char* fragmentShader = (const char*)LockResource(hFragmentShaderGlobal);

    const ShaderProgram& shader = *(shaders[name] = std::move(create(vertexShader, (GLint)vertexShaderSize, fragmentShader, (GLint)fragmentShaderSize)));

    FreeResource(hVertexShaderGlobal);
    FreeResource(hFragmentShaderGlobal);

    return shader;
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

GLint ShaderProgram::getUniformLocation(const std::string& name) const
{
    auto pair = uniforms.find(name);

    if (pair != uniforms.end())
        return pair->second;

    return glGetUniformLocation(id, name.c_str());
}

void ShaderProgram::set(const std::string& name, const GLint value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void ShaderProgram::set(const std::string& name, const GLfloat value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void ShaderProgram::set(const std::string& name, const bool value) const
{
    glUniform1i(getUniformLocation(name), (GLint)value);
}

void ShaderProgram::set(const std::string& name, const Vector2& value) const
{
    glUniform2fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const std::string& name, const Vector3& value) const
{
    glUniform3fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const std::string& name, const Vector4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const std::string& name, const Color4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, value.data());
}

void ShaderProgram::set(const std::string& name, const Matrix4& value) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, false, value.data());
}
