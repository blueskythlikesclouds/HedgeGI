#pragma once

#include "Frustum.h"

class Camera
{
public:
    Vector3 position;
    Quaternion rotation;
    Vector3 direction;
    Matrix4 view;
    Matrix4 projection;
    Frustum frustum;
    float aspectRatio;
    float fieldOfView;

    Camera();

    void computeValues();
    Vector3 getNewObjectPosition() const;
};
