#pragma once

class MetaInstancer
{
public:
    class Instance
    {
    public:
        Vector3 position{};
        uint8_t type{};

        float sway{};

        float pitchAfterSway{};
        float yawAfterSway{};

        float pitchBeforeSway{};
        float yawBeforeSway{};

        Color4 color{};
    };

    std::string name;
    std::vector<Instance> instances;

    void read(hl::stream& stream);
    void save(hl::stream& stream) const;
    void save(const std::string& filePath) const;
};
