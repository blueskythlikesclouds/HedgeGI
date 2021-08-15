#pragma once

class SHLightField
{
public:
    std::string name {};
    Eigen::Array3i resolution {};
    Vector3 position {};
    Vector3 rotation {};
    Vector3 scale {};

    static void save(hl::stream& stream, const std::vector<std::unique_ptr<SHLightField>>& shLightFields);
    static void save(const std::string& filePath, const std::vector<std::unique_ptr<SHLightField>>& shLightFields);

    Matrix3 getRotationMatrix() const;
    void setFromRotationMatrix(const Matrix3& matrix);
    Matrix4 getMatrix() const;
};