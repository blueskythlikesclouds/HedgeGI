#pragma once

class ShaderProgram
{
    static std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> shaders;
    static GLuint create(GLenum type, const GLchar* string, GLint length);

    std::unordered_map<const char*, GLint> uniforms;

    void registerUniforms();

public:
    static std::unique_ptr<ShaderProgram> create(const GLchar* vertexShader, GLint vertexShaderLength, const GLchar* fragmentShader, GLint fragmentShaderLength);

    static const ShaderProgram& get(const char* name);

    const GLuint id;

    ShaderProgram(GLuint id);
    ~ShaderProgram();

    void use() const;

    GLint getUniformLocation(const char* name) const;

    void set(const char* name, GLint value) const;
    void set(const char* name, GLfloat value) const;
    void set(const char* name, bool value) const;
    void set(const char* name, const Vector2& value) const;
    void set(const char* name, const Vector3& value) const;
    void set(const char* name, const Vector4& value) const;
    void set(const char* name, const Color4& value) const;
    void set(const char* name, const Matrix4& value) const;
};
