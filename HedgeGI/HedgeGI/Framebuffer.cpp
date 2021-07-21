#include "Framebuffer.h"
#include "Texture.h"

Framebuffer::Framebuffer(GLsizei width, GLsizei height) : id(glGenFramebuffer()), width(width), height(height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &id);
}

void Framebuffer::bind() const
{
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void Framebuffer::attach(GLenum attachment, const Texture& texture) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture.target, texture.id, 0);
}

FramebufferTexture::FramebufferTexture(GLenum attachment, GLenum target, GLint internalformat, GLsizei width,
    GLsizei height, GLenum format, GLenum type) : Framebuffer(width, height), texture(target, internalformat, width, height, format, type)
{
    attach(attachment, texture);
}
