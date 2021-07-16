#pragma once

class SHLightField
{
public:
    std::string name {};
    Eigen::Array3i resolution {};
    Matrix4 matrix {};

    void read(const FileStream& file);
    void write(const FileStream& file) const;
};