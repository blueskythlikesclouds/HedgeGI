#pragma once

class SHLightField
{
public:
    std::string name {};
    Eigen::Array3i resolution {};
    Vector3 position {};
    Vector3 rotation {};
    Vector3 scale {};

    Matrix3 getRotationMatrix() const;
    void setFromRotationMatrix(const Matrix3& matrix);
    Matrix4 getMatrix() const;
};