#pragma once

class Camera
{
public:
    Vector3 position;
    Quaternion rotation;
    float aspectRatio;
    float fieldOfView;

    Camera();

    Vector3 getDirection() const;
    Matrix4 getView() const;
    Matrix4 getProjection() const;

    Vector3 getNewObjectPosition() const;
};