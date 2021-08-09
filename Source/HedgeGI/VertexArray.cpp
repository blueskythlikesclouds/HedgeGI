#include "VertexArray.h"

VertexArray::VertexArray(GLsizeiptr size, const void* data, const std::initializer_list<VertexAttribute>& attributes)
    : id(glGenVertexArray()), buffer(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW)
{
    glBindVertexArray(id);

    for (auto& attribute : attributes)
    {
        glVertexAttribPointer(attribute.index, attribute.size, attribute.type, attribute.normalized, attribute.stride, (const void*)attribute.pointer);
        glEnableVertexAttribArray(attribute.index);
    }
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &id);
}

void VertexArray::bind() const
{
    glBindVertexArray(id);
}
