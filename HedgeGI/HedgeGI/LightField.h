#pragma once

struct LightFieldProbe
{
    uint8_t colors[8][3];
    uint8_t shadow;
};

enum LightFieldCellType
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
    AxisAlignedBoundingBox aabb;
    std::vector<LightFieldCell> cells;
    std::vector<LightFieldProbe> probes;

    LightFieldCell* createCell();
    LightFieldProbe* createProbe();

    void write(hl::File& file, hl::OffsetTable& offsetTable) const;
    void save(const std::string& filePath) const;
};