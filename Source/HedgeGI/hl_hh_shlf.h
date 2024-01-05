#pragma once

namespace hl::hh::needle
{
    struct raw_sh_light_field_node
    {
        off64<char> name;
        u32 probeCounts[3];
        vec3 position;
        vec3 rotation;
        vec3 scale;
    };

    struct raw_sh_light_field
    {
        u32 version;
        u8 unknown[0x90];
        u32 count;
        off64<raw_sh_light_field_node> entries;

        void fix(bina::endian_flag flag)
        {
        }
    };
}