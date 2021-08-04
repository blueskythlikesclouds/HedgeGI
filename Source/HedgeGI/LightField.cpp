#include "LightField.h"
#include "Utilities.h"

template <typename T, typename... Rest>
void hashCombine(size_t& seed, const T& v, const Rest&... rest)
{
    seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}

namespace std
{
    template<>
    struct hash<LightFieldProbe>
    {
        size_t operator()(const LightFieldProbe& probe) const noexcept
        {
            size_t hash = 0;

            hashCombine(hash,
                probe.colors[0][0], probe.colors[0][1], probe.colors[0][2],
                probe.colors[1][0], probe.colors[1][1], probe.colors[1][2],
                probe.colors[2][0], probe.colors[2][1], probe.colors[2][2],
                probe.colors[3][0], probe.colors[3][1], probe.colors[3][2],
                probe.colors[4][0], probe.colors[4][1], probe.colors[4][2],
                probe.colors[5][0], probe.colors[5][1], probe.colors[5][2],
                probe.colors[6][0], probe.colors[6][1], probe.colors[6][2],
                probe.colors[7][0], probe.colors[7][1], probe.colors[7][2], probe.shadow);

            return hash;
        }
    };
}

void LightField::optimizeProbes()
{
    phmap::parallel_flat_hash_map<LightFieldProbe, uint32_t> map;

    for (auto& probe : probes)
    {
        if (map.find(probe) != map.end())
            continue;

        map[probe] = (uint32_t)map.size();
    }

    std::vector<LightFieldProbe> newProbes;
    newProbes.resize(map.size());

    for (auto& pair : map)
        newProbes[pair.second] = pair.first;

    for (size_t i = 0; i < indices.size(); i++)
        indices[i] = map[probes[indices[i]]];

    std::swap(probes, newProbes);
}

void LightField::write(hl::stream& stream, hl::off_table& offTable) const
{
    struct LightFieldHeader
    {
        float aabb[6];
        hl::u32 cellCount;
        hl::u32 cellsOffset;
        hl::u32 probeCount;
        hl::u32 probesOffset;
        hl::u32 indexCount;
        hl::u32 indicesOffset;
    } header;

    offTable.push_back(sizeof(hl::hh::mirage::raw_header) + offsetof(LightFieldHeader, cellsOffset));
    offTable.push_back(sizeof(hl::hh::mirage::raw_header) + offsetof(LightFieldHeader, probesOffset));
    offTable.push_back(sizeof(hl::hh::mirage::raw_header) + offsetof(LightFieldHeader, indicesOffset));

    for (size_t i = 0; i < 3; i++)
    {
        header.aabb[i * 2 + 0] = aabb.min()[i];
        header.aabb[i * 2 + 1] = aabb.max()[i];

        hl::endian_swap(header.aabb[i * 2 + 0]);
        hl::endian_swap(header.aabb[i * 2 + 1]);
    }

    const size_t cellsOffset = sizeof(LightFieldHeader);
    const size_t probesOffset = cellsOffset + sizeof(LightFieldCell) * cells.size();
    const size_t indicesOffset = probesOffset + sizeof(LightFieldProbe) * probes.size();

    header.cellCount = (hl::u32)cells.size();
    header.cellsOffset = (hl::u32)cellsOffset;
    header.probeCount = (hl::u32)probes.size();
    header.probesOffset = (hl::u32)probesOffset;
    header.indexCount = (hl::u32)indices.size();
    header.indicesOffset = (hl::u32)indicesOffset;

    hl::endian_swap(header.cellCount);
    hl::endian_swap(header.cellsOffset);
    hl::endian_swap(header.probeCount);
    hl::endian_swap(header.probesOffset);
    hl::endian_swap(header.indexCount);
    hl::endian_swap(header.indicesOffset);
    
    stream.write_obj(header);

    for (auto& cell : cells)
    {
        hl::u32 type = (hl::u32)cell.type;
        hl::u32 index = (hl::u32)cell.index;

        hl::endian_swap(type);
        hl::endian_swap(index);

        stream.write_obj(type);
        stream.write_obj(index);
    }

    for (auto& probe : probes)
        stream.write_obj(probe);

    for (auto& index : indices)
    {
        hl::u32 value = index;
        hl::endian_swap(value);

        stream.write_obj(value);
    }
}

void LightField::save(const std::string& filePath) const
{
    hl::off_table offTable;

    hl::file_stream stream(toNchar(filePath.c_str()).data(), hl::file::mode::write);

    hl::hh::mirage::raw_header::start_write(1, stream);
    {
        write(stream, offTable);
    }
    hl::hh::mirage::raw_header::finish_write(0, offTable, stream);
}
