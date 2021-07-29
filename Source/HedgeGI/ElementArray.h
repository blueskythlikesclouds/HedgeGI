#pragma once

#include "Buffer.h"

class ElementArray
{
public:
    const Buffer buffer;
    const GLenum mode;
    const GLsizei count;
    const GLenum type;

    ElementArray(GLsizeiptr size, const void* data, GLenum mode, GLsizei count, GLenum type);

    void bind() const;
    void draw() const;
};
