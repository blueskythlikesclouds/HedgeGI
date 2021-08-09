#pragma once

#include "Buffer.h"

inline GLuint glGenVertexArray()
{
    GLuint array{};
    glGenVertexArrays(1, &array);
    return array;
}

struct VertexAttribute
{
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLsizeiptr pointer;
};

class VertexArray
{
public:
    const GLuint id;
    const Buffer buffer;

    VertexArray(GLsizeiptr size, const void* data, const std::initializer_list<VertexAttribute>& attributes);
    ~VertexArray();

    void bind() const;
};
