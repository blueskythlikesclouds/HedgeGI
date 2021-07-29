#pragma once

class Application;
class PropertyBag;

class Camera
{
public:
    Vector3 position;
    Quaternion rotation;
    float aspectRatio {};
    float fieldOfView {};

    struct
    {
        Vector3 position{};
        Quaternion rotation{};
        float aspectRatio {};
        float fieldOfView {};
    } history{};

    Camera();

    bool hasChanged() const;
    void setFieldOfView(float fieldOfView);

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;

    void update(const Application& application);
};
