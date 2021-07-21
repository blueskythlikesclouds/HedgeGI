#include "Texture.h"

Texture::Texture(GLenum target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type)
    : id(glGenTexture()), target(target), width(width), height(height)
{
    glBindTexture(target, id);
    glTexImage2D(target, 0, internalformat, width, height, 0, format, type, nullptr);
}

Texture::~Texture()
{
    glDeleteTextures(1, &id);
}

void Texture::bind() const
{
    glBindTexture(target, id);
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const
{
    glBindTexture(target, id);
    glTexSubImage2D(target, 0, xoffset, yoffset, width, height, format, type, pixels);
}
