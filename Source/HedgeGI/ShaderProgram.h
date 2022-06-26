#pragma once

class ShaderProgram
{
    static std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> shaders;
    static GLuint create(GLenum type, const char* defines, const char* shader, size_t shaderSize);

    std::unordered_map<const char*, GLint> uniforms;

    void registerUniforms();

public:
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
