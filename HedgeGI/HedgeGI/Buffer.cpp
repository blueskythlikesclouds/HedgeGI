#include "Buffer.h"

Buffer::Buffer(GLenum target, GLsizeiptr size, const void* data, GLenum usage) : id(glGenBuffer()), target(target), size(size), data(data), usage(usage)
{
    glBindBuffer(target, id);
    glBufferData(target, size, data, usage);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &id);
}

void Buffer::bind() const
{
    glBindBuffer(target, id);
}
