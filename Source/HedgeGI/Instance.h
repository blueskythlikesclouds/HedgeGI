#pragma once

class PropertyBag;
class Mesh;

class Instance
{
public:
    std::string name;
    std::vector<const Mesh*> meshes;
    AABB aabb;
    uint16_t originalResolution{};

    void buildAABB();

    uint16_t getResolution(const PropertyBag& propertyBag) const;
    void setResolution(PropertyBag& propertyBag, uint16_t resolution);
};
