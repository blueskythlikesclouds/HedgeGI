#pragma once

#include "Utilities.h"

// Ensures hash gets computed compile-time for read-only strings
#define PROP(x) \
    std::integral_constant<uint64_t, strHash(x)>::value

class FileStream;

struct Property
{
    uint64_t key{};
    uint64_t value{};
};

struct StringProperty
{
    uint64_t key {};
    std::string value;
};

class PropertyBag
{
public:
    std::vector<Property> properties;
    std::vector<StringProperty> stringProperties;

    template<typename T>
    T get(const uint64_t hash, const T defaultValue = T()) const
    {
        static_assert(sizeof(T) <= 8, "Properties can have values of 8 bytes max");

        for (auto& property : properties)
        {
            if (property.key == hash)
                return *(T*)&property.value;
        }

        return defaultValue;
    }

    template<typename T>
    T get(const char* name, const T defaultValue = T()) const
    {
        return get(strHash(name), defaultValue);
    }

    template<typename T>
    T get(const std::string& name, const T defaultValue = T()) const
    {
        return get(strHash(name.c_str()), defaultValue);
    }

    const std::string& getString(const uint64_t hash, const std::string& defaultValue = "") const
    {
        for (auto& property : stringProperties)
        {
            if (property.key == hash)
                return property.value;
        }

        return defaultValue;
    }

    const std::string& getString(const char* name, const std::string& defaultValue = "") const
    {
        return getString(strHash(name), defaultValue);
    }

    const std::string& getString(const std::string& name, const std::string& defaultValue = "") const
    {
        return getString(strHash(name.c_str()), defaultValue);
    }

    template<typename T>
    void set(const uint64_t hash, const T value)
    {
        static_assert(sizeof(T) <= 8, "Properties can have values of 8 bytes max");

        for (auto& property : properties)
        {
            if (property.key != hash)
                continue;

            *(T*)&property.value = value;
            return;
        }

        Property property = { hash };
        *(T*)&property.value = value;

        properties.push_back(std::move(property));
    }

    template<typename T>
    void set(const char* name, const T value)
    {
        set(strHash(name), value);
    }

    template<typename T>
    void set(const std::string& name, const T value)
    {
        set(strHash(name.c_str()), value);
    }

    void setString(const uint64_t hash, const std::string& value)
    {
        for (auto& property : stringProperties)
        {
            if (property.key != hash)
                continue;

            property.value = value;
            return;
        }

        stringProperties.push_back({ hash, value });
    }

    void setString(const char* name, const std::string& value)
    {
        return setString(strHash(name), value);
    }

    void setString(const std::string& name, const std::string& value)
    {
        return setString(strHash(name.c_str()), value);
    }

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    void load(const std::string& filePath);
    void save(const std::string& filePath) const;
};
