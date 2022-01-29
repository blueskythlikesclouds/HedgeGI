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

    static void saveLightList(hl::stream& stream, const std::vector<std::unique_ptr<Light>>& lights);

    float computeIntensity() const;

    void save(hl::stream& stream) const;
};
