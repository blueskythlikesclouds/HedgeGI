#include "Texture.h"
#include "ImageUtil.h"

Texture::Texture(GLenum target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
    : id(glGenTexture()), target(target), width(width), height(height)
{
    glBindTexture(target, id);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(target, 0, internalformat, width, height, 0, format, type, pixels);
}

Texture::Texture(const int rc) : Texture(ImageUtil::load(rc))
{
}

Texture::Texture(const DirectX::ScratchImage& image) :
    Texture(GL_TEXTURE_2D, GL_RGBA, (GLsizei)image.GetMetadata().width, (GLsizei)image.GetMetadata().height, GL_RGBA, GL_UNSIGNED_BYTE, image.GetPixels())
{
}

Texture::~Texture()
{
    glDeleteTextures(1, &id);
}

void Texture::bind() const
{
    glBindTexture(target, id);
}

void Texture::bind(size_t index) const
{
    glActiveTexture((GLenum)(GL_TEXTURE0 + index));
    glBindTexture(target, id);
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const
{
    glBindTexture(target, id);
    glTexSubImage2D(target, 0, xoffset, yoffset, width, height, format, type, pixels);
}
