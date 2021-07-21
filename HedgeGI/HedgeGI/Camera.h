#pragma once

class Input;

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
    void update(const Input& input, float elapsedTime);
};
