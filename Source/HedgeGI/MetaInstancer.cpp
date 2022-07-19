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

    void endianSwap()
    {
        hl::endian_swap(signature);
        hl::endian_swap(version);
        hl::endian_swap(instanceCount);
        hl::endian_swap(instanceSize);
        hl::endian_swap(instancesOffset);
    }
};

struct MtiInstance
{
    float positionX;
    float positionY;
    float positionZ;

    uint8_t type;
    uint8_t sway;

    // Gets applied after sway animation
    uint8_t pitchAfterSway;
    uint8_t yawAfterSway;

    // Gets applied before sway animation
    int16_t pitchBeforeSway;
    int16_t yawBeforeSway;

    // Shadow
    uint8_t colorA;

    // GI color
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;

    void endianSwap()
    {
        hl::endian_swap(positionX);
        hl::endian_swap(positionY);
        hl::endian_swap(positionZ);
        hl::endian_swap(pitchBeforeSway);
        hl::endian_swap(yawBeforeSway);
    }
};

void MetaInstancer::read(hl::stream& stream)
{
    MtiHeader mtiHeader{};
    stream.read_obj(mtiHeader);
    mtiHeader.endianSwap();

    if (mtiHeader.instanceSize != sizeof(MtiInstance))
        return;

    stream.seek(hl::seek_mode::beg, mtiHeader.instancesOffset);

    for (size_t i = 0; i < mtiHeader.instanceCount; i++)
    {
        MtiInstance mtiInstance{};
        stream.read_obj(mtiInstance);
        mtiInstance.endianSwap();

        auto& instance = instances.emplace_back();

        instance.position.x() = mtiInstance.positionX;
        instance.position.y() = mtiInstance.positionY;
        instance.position.z() = mtiInstance.positionZ;
        instance.type = mtiInstance.type;

        instance.sway = (float)mtiInstance.sway / 255.0f;

        instance.pitchAfterSway = (2 * PI) * ((float)mtiInstance.pitchAfterSway / 255.0f);
        instance.yawAfterSway = (2 * PI) * ((float)mtiInstance.yawAfterSway / 255.0f);

        instance.pitchBeforeSway = PI * ((float)mtiInstance.pitchBeforeSway / (mtiInstance.pitchBeforeSway >= 0 ? 32767.0f : 32768.0f));
        instance.yawBeforeSway = PI * ((float)mtiInstance.yawBeforeSway / (mtiInstance.yawBeforeSway >= 0 ? 32767.0f : 32768.0f));

        instance.color.w() = (float) mtiInstance.colorA / 255.0f;
        instance.color.x() = (float) mtiInstance.colorR / 255.0f;
        instance.color.y() = (float) mtiInstance.colorG / 255.0f;
        instance.color.z() = (float) mtiInstance.colorB / 255.0f;
    }
    
}

void MetaInstancer::save(hl::stream& stream) const
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

    mtiHeader.endianSwap();
    stream.write_obj(mtiHeader);

    for (auto& instance : instances)
    {
        MtiInstance mtiInstance =
        {
            instance.position.x(), // positionX
            instance.position.y(), // positionY
            instance.position.z(), // positionZ
            instance.type, // type
            (uint8_t)meshopt_quantizeUnorm(instance.sway, 8), // sway
            (uint8_t)meshopt_quantizeUnorm(fmodf(instance.pitchAfterSway, 2 * PI) / (2 * PI), 8), // pitchAfterSway
            (uint8_t)meshopt_quantizeUnorm(fmodf(instance.yawAfterSway, 2 * PI) / (2 * PI), 8), // yawAfterSway
            (int16_t)meshopt_quantizeSnorm(instance.pitchBeforeSway / PI, 16), // pitchBeforeSway
            (int16_t)meshopt_quantizeSnorm(instance.yawBeforeSway / PI, 16), // yawBeforeSway
            (uint8_t)meshopt_quantizeUnorm(instance.color.w(), 8), // colorA
            (uint8_t)meshopt_quantizeUnorm(instance.color.x(), 8), // colorR
            (uint8_t)meshopt_quantizeUnorm(instance.color.y(), 8), // colorG
            (uint8_t)meshopt_quantizeUnorm(instance.color.z(), 8) // colorB
        };

        mtiInstance.endianSwap();
        stream.write_obj(mtiInstance);
    }
}

void MetaInstancer::save(const std::string& filePath) const
{
    hl::file_stream stream(toNchar(filePath.c_str()).data(), hl::file::mode::write);
    save(stream);
}
