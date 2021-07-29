#pragma once

inline GLuint glGenBuffer()
{
    GLuint buffer{};
    glGenBuffers(1, &buffer);
    return buffer;
}

class Buffer
{
public:
    const GLuint id;
    const GLenum target;
    const GLsizeiptr size;
    const void* const data;
    const GLenum usage;

    Buffer(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    ~Buffer();

    void bind() const;
};
