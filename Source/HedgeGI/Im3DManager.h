#pragma once

#include "Camera.h"
#include "Component.h"
#include "Quad.h"
#include "VertexArray.h"

class ViewportWindow;
class Texture;
class ShaderProgram;

struct Billboard
{
    Vector2 rectPos;
    Vector2 rectSize;
    Color4 color;
    const Texture* texture;
    float depth;
};

class Im3DManager final : public Component
{
    const ShaderProgram& billboardShader;
    const Quad billboardQuad;
    std::vector<Billboard> billboards;

    const ShaderProgram& im3dShader;
    const VertexArray im3dVertexArray;

    Camera camera;
    Vector3 rayPosition;
    Vector3 rayDirection;

    // Cache as this is going to be reused very often
    ViewportWindow* viewportWindow{};

public:
    Im3DManager();

    const Camera& getCamera() const;
    const Vector3& getRayPosition() const;
    const Vector3& getRayDirection() const;

    const Billboard* drawBillboard(const Vector3& position, float size, const Color4& color, const Texture& texture);
    bool checkCursorOnBillboard(const Billboard& billboard) const;

    void endFrame();

    void update(float deltaTime) override;
};

static const inline Im3d::Id IM3D_TRANSPARENT_DISCARD_ID = Im3d::MakeId("TRANSPARENT_DISCARD");