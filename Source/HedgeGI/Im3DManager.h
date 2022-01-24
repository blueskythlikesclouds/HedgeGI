#pragma once

#include "Camera.h"
#include "Component.h"
#include "VertexArray.h"

class ShaderProgram;

class Im3DManager final : public Component
{
    const ShaderProgram& shader;
    const VertexArray vertexArray;
    Camera camera;

public:
    Im3DManager();

    void endFrame();

    void update(float deltaTime) override;
};

static const inline Im3d::Id IM3D_TRANSPARENT_DISCARD_ID = Im3d::MakeId("TRANSPARENT_DISCARD");