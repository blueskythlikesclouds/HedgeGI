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

enum class LightFieldCellType : uint32_t
{
    X,
    Y,
    Z,
    Probe
};

struct LightFieldCell
{
    LightFieldCellType type;
    uint32_t index;
};

class LightField
{
public:
    AABB aabb;
    std::vector<LightFieldCell> cells;
    std::vector<LightFieldProbe> probes;
    std::vector<uint32_t> indices;

    void optimizeProbes();
    void clear(bool clearCells);

    void read(void* rawData);
    void write(hl::stream& stream, hl::off_table& offTable) const;
    void save(const std::string& filePath) const;
};