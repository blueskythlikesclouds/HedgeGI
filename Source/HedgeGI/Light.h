#pragma once

enum class LightType : uint32_t
{
    Directional,
    Point
};

class Light
{
public:
    std::string name;
    Vector3 position;
    Color3 color;
    LightType type{};
    Vector4 range;
    float shadowRadius{};
    bool castShadow = true;

    static void saveLightList(hl::stream& stream, const std::vector<std::unique_ptr<Light>>& lights);

    float computeIntensity() const;

    void save(hl::stream& stream) const;
};