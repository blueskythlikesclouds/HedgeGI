#pragma once

class MetaInstancer
{
public:
    class Instance
    {
    public:
        Vector3 position;
        uint8_t type;
        Quaternion rotation;
        Color4 color;

        Instance();
    };

    std::vector<Instance> instances;

    void save(hl::stream& stream);
    void save(const std::string& filePath);
};
