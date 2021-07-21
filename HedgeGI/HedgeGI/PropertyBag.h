#pragma once

struct Property
{
    uint64_t key{};
    uint64_t value{};
};

class PropertyBag
{
public:
    std::vector<Property> properties;

    template<typename T>
    T get(const std::string& name, const T defaultValue = T()) const
    {
        static_assert(sizeof(T) <= 8, "Properties can have values of 8 bytes max");

        const uint64_t hash = computeHash(name.c_str());
        for (auto& property : properties)
        {
            if (property.key == hash)
                return *(T*)&property.value;
        }

        return defaultValue;
    }

    template<typename T>
    void set(const std::string& name, const T value)
    {
        static_assert(sizeof(T) <= 8, "Properties can have values of 8 bytes max");

        const uint64_t hash = computeHash(name.c_str());

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

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    void load(const std::string& filePath);
    void save(const std::string& filePath) const;
};
