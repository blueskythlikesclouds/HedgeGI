#include "MetaInstancer.h"
#include "Math.h"
#include "Utilities.h"

struct MtiHeader
{
    uint32_t signature;
    uint32_t version;
    uint32_t instanceCount;
    uint32_t instanceSize;
    uint32_t unused0;
    uint32_t unused1;
    uint32_t unused2;
    uint32_t instancesOffset;
};

struct MtiInstance
{
    float positionX;
    float positionY;
    float positionZ;

    uint8_t type;
    uint8_t unknown0;

    int16_t rotationX;
    int16_t rotationY;
    int16_t rotationZ;

    uint8_t colorA;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
};

MetaInstancer::Instance::Instance()
    : position(Vector3::Zero()), type(0), rotation(Quaternion::Identity()), color(Color4::Ones())
{
}

void MetaInstancer::save(hl::stream& stream)
{
    MtiHeader mtiHeader =
    {
        0x4D544920, // signature
        1, // version
        (uint32_t)instances.size(), // instanceCount
        sizeof(MtiInstance), // instanceSize
        0, // unused0
        0, // unused1
        0, // unused2,
        sizeof(MtiHeader) // instancesOffset
    };

    hl::endian_swap(mtiHeader.signature);
    hl::endian_swap(mtiHeader.version);
    hl::endian_swap(mtiHeader.instanceCount);
    hl::endian_swap(mtiHeader.instanceSize);
    hl::endian_swap(mtiHeader.instancesOffset);

    stream.write_obj(mtiHeader);

    for (auto& instance : instances)
    {
        const Vector3 rotation = instance.rotation.toRotationMatrix().eulerAngles(0, 1, 2);

        MtiInstance mtiInstance =
        {
            instance.position.x(), // positionX
            instance.position.y(), // positionY
            instance.position.z(), // positionZ
            instance.type, // type
            0xFF, // unknown0,
            (int16_t)(rotation.x() / PI * 32767.0f), // rotationX
            (int16_t)(rotation.y() / PI * 32767.0f), // rotationY
            (int16_t)(rotation.z() / PI * 32767.0f), // rotationZ,
            (uint8_t)(instance.color.w() * 255.0f), // colorA
            (uint8_t)(instance.color.x() * 255.0f), // colorR
            (uint8_t)(instance.color.y() * 255.0f), // colorG
            (uint8_t)(instance.color.z() * 255.0f) // colorB
        };

        hl::endian_swap(mtiInstance.positionX);
        hl::endian_swap(mtiInstance.positionY);
        hl::endian_swap(mtiInstance.positionZ);
        hl::endian_swap(mtiInstance.rotationX);
        hl::endian_swap(mtiInstance.rotationY);
        hl::endian_swap(mtiInstance.rotationZ);

        stream.write_obj(mtiInstance);
    }
}

void MetaInstancer::save(const std::string& filePath)
{
    hl::file_stream stream(toNchar(filePath.c_str()).data(), hl::file::mode::write);
    save(stream);
}
