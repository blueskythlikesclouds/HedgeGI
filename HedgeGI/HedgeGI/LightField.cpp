#include "LightField.h"

template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
    seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}

namespace std
{
    template<>
    struct hash<LightFieldProbe>
    {
        size_t operator()(LightFieldProbe const& probe) const noexcept
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

LightFieldCell* LightField::createCell()
{
    return &cells.emplace_back();
}

LightFieldProbe* LightField::createProbe()
{
    return &probes.emplace_back();
}

void LightField::write(hl::File& file, hl::OffsetTable& offsetTable) const
{
    file.Write(aabb.begin[0]);
    file.Write(aabb.end[0]);
    file.Write(aabb.begin[1]);
    file.Write(aabb.end[1]);
    file.Write(aabb.begin[2]);
    file.Write(aabb.end[2]);

    const uint64_t tableOffset = file.Tell();
    file.WriteNulls(24);
}

void LightField::save(const std::string& filePath) const
{
    hl::File file(filePath.c_str(), hl::FileMode::WriteBinary);
    hl::OffsetTable offsetTable;
    hl::HHStartWriteStandard(file, 1);
    {
        write(file, offsetTable);
    }
    hl::HHFinishWriteStandard(file, 0, offsetTable, true);
}
