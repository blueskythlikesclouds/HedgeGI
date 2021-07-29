#include "PropertyBag.h"
#include "FileStream.h"

void PropertyBag::read(const FileStream& file)
{
    properties.resize(file.read<uint32_t>());
    stringProperties.resize(file.read<uint32_t>());
    file.read(properties.data(), properties.size());

    for (auto& property : stringProperties)
    {
        property.key = file.read<uint64_t>();
        property.value = file.readString();
    }
}

void PropertyBag::write(const FileStream& file) const
{
    file.write((uint32_t)properties.size());
    file.write((uint32_t)stringProperties.size());
    file.write(properties.data(), properties.size());

    for (auto& property : stringProperties)
    {
        file.write(property.key);
        file.write(property.value);
    }
}

void PropertyBag::load(const std::string& filePath)
{
    properties.clear();
    stringProperties.clear();

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
