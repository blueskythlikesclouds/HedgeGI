#include "ElementArray.h"

ElementArray::ElementArray(GLsizeiptr size, const void* data, GLenum mode, GLsizei count, GLenum type)
    : buffer(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW), mode(mode), count(count), type(type)
{
}

void ElementArray::bind() const
{
    buffer.bind();
}

void ElementArray::draw() const
{
    buffer.bind();
    glDrawElements(mode, count, type, nullptr);
}
