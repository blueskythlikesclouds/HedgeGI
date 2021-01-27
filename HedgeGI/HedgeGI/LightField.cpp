#include "LightField.h"

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
}

void LightField::write(hl::File& file, hl::OffsetTable& offsetTable) const
{
    // HedgeLib for some reason needs variables to be non-const for endian
    // swapping to work, so we have to do this.

    float minX = aabb.min().x();
    float maxX = aabb.max().x();
    float minY = aabb.min().y();
    float maxY = aabb.max().y();
    float minZ = aabb.min().z();
    float maxZ = aabb.max().z();

    file.Write(minX);
    file.Write(maxX);
    file.Write(minY);
    file.Write(maxY);
    file.Write(minZ);
    file.Write(maxZ);

    uint32_t cellCount = (uint32_t)cells.size();
    uint32_t probeCount = (uint32_t)probes.size();
    uint32_t indexCount = (uint32_t)indices.size();
     
    const size_t tableOffset = file.Tell();

    file.Write(cellCount);
    file.WriteNulls(4);
    file.Write(probeCount);
    file.WriteNulls(4);
    file.Write(indexCount);
    file.WriteNulls(4);

    // Cells
    file.FixOffset32(tableOffset + 4, file.Tell(), offsetTable);

    for (auto& cell : cells)
    {
        uint32_t type = (uint32_t)cell.type;
        uint32_t index = cell.index;

        file.Write(type);
        file.Write(index);
    }

    // Probes
    file.FixOffset32(tableOffset + 12, file.Tell(), offsetTable);

    for (auto& probe : probes)
        file.WriteBytes(&probe, sizeof(LightFieldProbe));

    // Indices
    file.FixOffset32(tableOffset + 20, file.Tell(), offsetTable);

    for (auto& i : indices)
    {
        uint32_t index = i;
        file.Write(index);
    }

    file.Align();
}

void LightField::save(const std::string& filePath) const
{
    hl::File file(filePath.c_str(), hl::FileMode::WriteBinary, true);
    hl::OffsetTable offsetTable;
    hl::HHStartWriteStandard(file, 1);
    {
        write(file, offsetTable);
    }
    hl::HHFinishWriteStandard(file, 0, offsetTable, true);
}
