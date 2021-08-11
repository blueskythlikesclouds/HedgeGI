#pragma once

enum class LightType
{
    Directional,
    Point
};

class Light
{
public:
    std::string name;
    LightType type{};
    Vector3 position;
    Color3 color;
    Vector4 range;

    Matrix3 getTangentToWorldMatrix() const;
};
