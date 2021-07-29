#pragma once

namespace hl::hh::mirage
{
    enum class light_type : u32
    {
        directional = 0,
        point = 1
    };

    struct raw_light
    {
        light_type type;
        vec3 position;
        vec3 color;
        u32 attribute;
        vec4 range;

        template<bool swapOffsets = true>
        void endian_swap() noexcept
        {
            hl::endian_swap(*(u32*)&type);
            hl::endian_swap(position);
            hl::endian_swap(color);

            if (type != light_type::point)
                return;

            hl::endian_swap(attribute);
            hl::endian_swap(range);
        }
    };
}