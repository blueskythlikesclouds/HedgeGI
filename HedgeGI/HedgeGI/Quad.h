#pragma once

#include "ElementArray.h"
#include "VertexArray.h"

class Quad
{
    static Vector3 vertices[];
    static uint8_t indices[];

public:
    const VertexArray vertexArray;
    const ElementArray elementArray;

    Quad();
};
