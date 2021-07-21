#pragma once

class Application;

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

    void update(const Application& application);
};
