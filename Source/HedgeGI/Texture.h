#pragma once

inline GLuint glGenTexture()
{
    GLuint texture{};
    glGenTextures(1, &texture);
    return texture;
}

class Texture
{
    Texture(const DirectX::ScratchImage& image);

public:
    const GLuint id;
    const GLenum target;
    const GLsizei width;
    const GLsizei height;

    Texture(GLenum target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels = nullptr);
    Texture(int rc);
    ~Texture();

    void bind() const;
    void bind(size_t index) const;
    void subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const;
};
