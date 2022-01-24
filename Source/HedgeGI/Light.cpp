#include "Light.h"

void Light::saveLightList(hl::stream& stream, const std::vector<std::unique_ptr<Light>>& lights)
{
    hl::off_table offTable;
    hl::hh::mirage::raw_header::start_write(0, stream);

    const size_t basePos = stream.tell();

    stream.write_obj(HL_SWAP_U32(lights.size()));
    stream.write_nulls(4);

    const size_t offsetsPos = stream.tell();

    stream.fix_off32(basePos, basePos + 4, hl::hh::mirage::needs_endian_swap, offTable);
    stream.write_nulls(4 * lights.size());

    for (size_t i = 0; i < lights.size(); i++)
    {
        stream.fix_off32(basePos, offsetsPos + 4 * i, hl::hh::mirage::needs_endian_swap, offTable);
        stream.write_str(lights[i]->name);
        stream.pad(4);
    }

    hl::hh::mirage::raw_header::finish_write(0, offTable, stream, "");
}

void Light::save(hl::stream& stream) const
{
    hl::off_table offTable;
    hl::hh::mirage::raw_header::start_write(1, stream);

    hl::hh::mirage::raw_light hhLight =
    {
        (hl::hh::mirage::light_type)type,
        { position.x(), position.y(), position.z() },
        { color.x(), color.y(), color.z() },
        0,
        { range.x(), range.y(), range.z(), range.w() }
    };

    hl::endian_swap(*(hl::u32*)&hhLight.type);
    hl::endian_swap(hhLight.position);
    hl::endian_swap(hhLight.color);
    hl::endian_swap(hhLight.attribute);
    hl::endian_swap(hhLight.range);

    if (type == LightType::Point)
        stream.write(sizeof(hl::hh::mirage::raw_light), &hhLight);

    else
        stream.write(offsetof(hl::hh::mirage::raw_light, attribute), &hhLight);

    hl::hh::mirage::raw_header::finish_write(0, offTable, stream, "");
}