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

    Vector3 getDirection() const;
    Matrix4 getView() const;
    Matrix4 getProjection() const;

    Vector3 getNewObjectPosition() const;

    bool hasChanged() const;
    void setFieldOfView(float fieldOfView);

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;

    void update(const Application& application);
};
