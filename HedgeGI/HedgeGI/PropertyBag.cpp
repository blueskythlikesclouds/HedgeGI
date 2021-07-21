#include "PropertyBag.h"

void PropertyBag::read(const FileStream& file)
{
    properties.resize(file.read<uint64_t>());
    file.read(properties.data(), properties.size());
}

void PropertyBag::write(const FileStream& file) const
{
    file.write((uint64_t)properties.size());
    file.write(properties.data(), properties.size());
}

void PropertyBag::load(const std::string& filePath)
{
    const FileStream fileStream(filePath.c_str(), "rb");

    if (fileStream.isOpen())
        read(fileStream);
}

void PropertyBag::save(const std::string& filePath) const
{
    const FileStream fileStream(filePath.c_str(), "wb");

    if (fileStream.isOpen())
        write(fileStream);
}
