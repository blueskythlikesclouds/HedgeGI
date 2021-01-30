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

void LightField::write(HlStream* stream, HlOffTable* offTable) const
{
    struct LightFieldHeader
    {
        float aabb[6];
        HlU32 cellCount;
        HL_OFF32(LightFieldCell) cellsOffset;
        HlU32 probeCount;
        HL_OFF32(LightFieldProbe) probesOffset;
        HlU32 indexCount;
        HL_OFF32(HlU32) indicesOffset;
    } header;

    HL_LIST_PUSH(*offTable, sizeof(HlHHStandardHeader) + offsetof(LightFieldHeader, cellsOffset));
    HL_LIST_PUSH(*offTable, sizeof(HlHHStandardHeader) + offsetof(LightFieldHeader, probesOffset));
    HL_LIST_PUSH(*offTable, sizeof(HlHHStandardHeader) + offsetof(LightFieldHeader, indicesOffset));

    for (size_t i = 0; i < 3; i++)
    {
        header.aabb[i * 2 + 0] = hlSwapFloat(aabb.min()[i]);
        header.aabb[i * 2 + 1] = hlSwapFloat(aabb.max()[i]);
    }

    const size_t cellsOffset = sizeof(LightFieldHeader);
    const size_t probesOffset = cellsOffset + sizeof(LightFieldCell) * cells.size();
    const size_t indicesOffset = probesOffset + sizeof(LightFieldProbe) * probes.size();

    header.cellCount = hlSwapU32((HlU32)cells.size());
    header.cellsOffset = hlSwapU32((HlU32)cellsOffset);
    header.probeCount = hlSwapU32((HlU32)probes.size());
    header.probesOffset = hlSwapU32((HlU32)probesOffset);
    header.indexCount = hlSwapU32((HlU32)indices.size());
    header.indicesOffset = hlSwapU32((HlU32)indicesOffset);
    
    hlStreamWrite(stream, sizeof(LightFieldHeader), &header, nullptr);

    for (auto& cell : cells)
    {
        HlU32 type = hlSwapU32((HlU32)cell.type);
        HlU32 index = hlSwapU32((HlU32)cell.index);

        hlStreamWrite(stream, sizeof(HlU32), &type, nullptr);
        hlStreamWrite(stream, sizeof(HlU32), &index, nullptr);
    }

    for (auto& probe : probes)
        hlStreamWrite(stream, sizeof(LightFieldProbe), &probe, nullptr);

    for (auto& index : indices)
    {
        HlU32 value = hlSwapU32(index);
        hlStreamWrite(stream, sizeof(HlU32), &value, nullptr);
    }
}

void LightField::save(const std::string& filePath) const
{
    HlOffTable offTable;
    HL_LIST_INIT(offTable);

    HlNChar nFilePath[MAX_PATH];
    hlStrConvUTF8ToNativeNoAlloc(filePath.c_str(), nFilePath, 0, MAX_PATH);

    HlFileStream* stream;
    hlFileStreamOpen(nFilePath, HL_FILE_MODE_WRITE, &stream);

    hlHHStandardStartWrite(stream, 1);
    {
        write(stream, &offTable);
    }
    hlHHStandardFinishWrite(0, true, &offTable, stream);

    HL_LIST_FREE(offTable);
    hlFileStreamClose(stream);
}
