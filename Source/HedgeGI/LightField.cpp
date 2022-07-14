#include "LightField.h"
#include "Utilities.h"

struct LightFieldHeader
{
    float aabb[6];
    hl::arr32<LightFieldCell> cells;
    hl::arr32<LightFieldProbe> probes;
    hl::arr32<uint32_t> indices;
};

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

void LightField::clear(const bool clearCells)
{
    if (clearCells)
        cells.clear();

    probes.clear();
    indices.clear();
}

void LightField::read(void* rawData)
{
    hl::hh::mirage::fix(rawData);

    const LightFieldHeader* header = hl::hh::mirage::get_data<LightFieldHeader>(rawData);

    for (size_t i = 0; i < 3; i++)
    {
        aabb.min()[i] = header->aabb[i * 2 + 0];
        aabb.max()[i] = header->aabb[i * 2 + 1];

        hl::endian_swap(aabb.min()[i]);
        hl::endian_swap(aabb.max()[i]);
    }

    clear(true);

    cells.resize(HL_SWAP_U32(header->cells.count));

    for (size_t i = 0; i < cells.size(); i++)
    {
        const LightFieldCell& srcCell = header->cells.get()[i];
        LightFieldCell& dstCell = cells[i];

        dstCell.type = (LightFieldCellType)HL_SWAP_U32(srcCell.type);
        dstCell.index = HL_SWAP_U32(srcCell.index);
    }

    // We never need probes or indices, so skip parsing them.
}

void LightField::write(hl::stream& stream, hl::off_table& offTable) const
{
    LightFieldHeader header {};

    offTable.push_back(sizeof(hl::hh::mirage::standard::raw_header) + offsetof(LightFieldHeader, cells.data));
    offTable.push_back(sizeof(hl::hh::mirage::standard::raw_header) + offsetof(LightFieldHeader, probes.data));
    offTable.push_back(sizeof(hl::hh::mirage::standard::raw_header) + offsetof(LightFieldHeader, indices.data));

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

    header.cells.count = (hl::u32)cells.size();
    header.cells.data = (hl::u32)cellsOffset;
    header.probes.count = (hl::u32)probes.size();
    header.probes.data = (hl::u32)probesOffset;
    header.indices.count = (hl::u32)indices.size();
    header.indices.data = (hl::u32)indicesOffset;

    hl::endian_swap(header.cells);
    hl::endian_swap(header.probes);
    hl::endian_swap(header.indices);
    
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

    hl::hh::mirage::standard::raw_header::start_write(stream);
    {
        write(stream, offTable);
    }
    hl::hh::mirage::standard::raw_header::finish_write(0, sizeof(hl::hh::mirage::standard::raw_header), 1, offTable, stream);
}
