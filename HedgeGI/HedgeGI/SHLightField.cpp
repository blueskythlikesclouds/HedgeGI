#include "SHLightField.h"

void SHLightField::read(const FileStream& file)
{
    name = file.readString();
    resolution = file.read<Eigen::Array3i>();
    matrix = file.read<Matrix4>();
}

void SHLightField::write(const FileStream& file) const
{
    file.write(name);
    file.write(resolution);
    file.write(matrix);
}
