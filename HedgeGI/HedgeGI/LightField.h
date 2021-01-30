#pragma once

struct LightFieldProbe
{
    uint8_t colors[8][3];
    uint8_t shadow;

    bool operator==(const LightFieldProbe& other) const noexcept
    {
        return memcmp(this, &other, sizeof(LightFieldProbe)) == 0;
    }
};

enum LightFieldCellType : uint32_t
{
    LIGHT_FIELD_CELL_TYPE_X,
    LIGHT_FIELD_CELL_TYPE_Y,
    LIGHT_FIELD_CELL_TYPE_Z,
    LIGHT_FIELD_CELL_TYPE_PROBE
};

struct LightFieldCell
{
    LightFieldCellType type;
    uint32_t index;
};

class LightField
{
public:
    Eigen::AlignedBox3f aabb;
    std::vector<LightFieldCell> cells;
    std::vector<LightFieldProbe> probes;
    std::vector<uint32_t> indices;

    void optimizeProbes();

    void write(HlStream* stream, HlOffTable* offTable) const;
    void save(const std::string& filePath) const;
};