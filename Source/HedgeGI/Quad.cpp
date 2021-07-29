#include "Quad.h"

Vector3 Quad::vertices[] =
{
    Vector3(-1.0f, -1.0f, 0.0f),
    Vector3(-1.0f, 1.0f, 0.0f),
    Vector3(1.0f, 1.0f, 0.0f),
    Vector3(1.0f, -1.0f, 0.0f)
};

uint8_t Quad::indices[] =
{
    0, 2, 1,
    0, 3, 2
};

Quad::Quad() : vertexArray(sizeof(vertices), vertices, {{0, 3, GL_FLOAT, sizeof(Vector3), 0}}),
    elementArray(sizeof(indices), indices, GL_TRIANGLES, _countof(indices), GL_UNSIGNED_BYTE)
{
}
