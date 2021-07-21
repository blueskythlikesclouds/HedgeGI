#pragma once

#include "Texture.h"

inline GLuint glGenFramebuffer()
{
    GLuint framebuffer{};
    glGenFramebuffers(1, &framebuffer);
    return framebuffer;
}

class Framebuffer
{
public:
    const GLuint id;
    const GLsizei width;
    const GLsizei height;

    Framebuffer(GLsizei width, GLsizei height);
    ~Framebuffer();

    void bind() const;
    void attach(GLenum attachment, const Texture& texture) const;
};

class FramebufferTexture : public Framebuffer
{
public:
    const Texture texture;

    FramebufferTexture(GLenum attachment, GLenum target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type);
};