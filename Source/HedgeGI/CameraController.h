#pragma once

#include "Camera.h"
#include "Component.h"

class PropertyBag;

class CameraController final : public Component
{
public:
    Camera current;
    Camera previous;

    bool hasChanged() const;

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;

    void update(float deltaTime) override;
};
