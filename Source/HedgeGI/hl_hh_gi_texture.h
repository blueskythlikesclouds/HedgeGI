#pragma once

namespace hl
{
namespace hh
{
namespace mirage
{

class atlas_texture
{
public:
    std::string name;
    float width;
    float height;
    float x;
    float y;

    HL_API void write(stream& stream) const noexcept
    {
        const u8 nameSize = static_cast<u8>(name.size());
        const u8 width = static_cast<u8>(std::log2f(1.0f / this->width));
        const u8 height = static_cast<u8>(std::log2f(1.0f / this->height));
        const u8 x = static_cast<u8>(this->x * 256);
        const u8 y = static_cast<u8>(this->y * 256);

        stream.write_obj(nameSize);
        stream.write(name.size(), name.data());
        stream.write_obj(width);
        stream.write_obj(height);
        stream.write_obj(x);
        stream.write_obj(y);
    }
};

class atlas
{
public:
    std::string name;
    std::vector<atlas_texture> textures;

    HL_API void write(stream& stream) const noexcept
    {
        const u8 nameSize = static_cast<u8>(name.size());
        const u16 textureCount = static_cast<u16>(textures.size());

        stream.write_obj(nameSize);
        stream.write(name.size(), name.data());
        stream.write_obj(textureCount);

        for (auto& texture : textures)
            texture.write(stream);
    }
};

class atlas_info
{
public:
    std::vector<atlas> atlases;

    HL_API void write(stream& stream) const noexcept
    {
        const u8 empty = static_cast<u8>(atlases.empty());
        stream.write_obj(empty);

        if (empty) return;

        const u16 atlasCount = static_cast<u16>(atlases.size());
        stream.write_obj(atlasCount);

        for (auto& atlas : atlases)
            atlas.write(stream);
    }
};

struct raw_gi_texture_group
{
    u32 level;
    arr32<u32> indices;
    off32<bounding_sphere> bounds;
    u32 memorySize;

    template<bool swapOffsets = true>
    void endian_swap() noexcept
    {
        hl::endian_swap(level);
        hl::endian_swap<swapOffsets>(indices);
        hl::endian_swap<swapOffsets>(bounds);
        hl::endian_swap(memorySize);
    }

    HL_API void fix() noexcept
    {
#ifndef HL_IS_BIG_ENDIAN
        endian_swap<false>();

        for (auto& index : indices)
            hl::endian_swap(index);

        hl::endian_swap<false>(*bounds);
#endif
    }
};

HL_STATIC_ASSERT_SIZE(raw_gi_texture_group, 0x14);

struct raw_gi_texture_group_info_v2
{
    u32 instanceCount;
    off32<off32<char>> instanceNames;
    off32<off32<bounding_sphere>> instanceBounds;
    arr32<off32<raw_gi_texture_group>> groups;
    arr32<u32> level2GroupIndices;

    template<bool swapOffsets = true>
    void endian_swap() noexcept
    {
        hl::endian_swap(instanceCount);
        hl::endian_swap<swapOffsets>(instanceNames);
        hl::endian_swap<swapOffsets>(instanceBounds);
        hl::endian_swap<swapOffsets>(groups);
        hl::endian_swap<swapOffsets>(level2GroupIndices);
    }

    HL_API void fix() noexcept
    {
#ifndef HL_IS_BIG_ENDIAN
        endian_swap<false>();

        for (auto& group : groups)
            group->fix();

        for (auto& index : level2GroupIndices)
            hl::endian_swap(index);
#endif
    }
};

HL_STATIC_ASSERT_SIZE(raw_gi_texture_group_info_v2, 0x1C);

}
}
}