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
    const LightFieldCell* left;
    const LightFieldCell* right;
    const LightFieldProbe* probes[8];
};

class LightField
{
public:
    Eigen::AlignedBox3f aabb;
    std::vector<std::unique_ptr<LightFieldCell>> cells;
    std::vector<std::unique_ptr<LightFieldProbe>> probes;

    LightFieldCell* createCell();
    LightFieldProbe* createProbe();

    size_t getIndex(const LightFieldCell* cell) const;
    size_t getIndex(const LightFieldProbe* probe) const;

    void optimizeProbes();

    void write(hl::File& file, hl::OffsetTable& offsetTable) const;
    void save(const std::string& filePath) const;
};