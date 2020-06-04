#pragma once

class Bitmap;
class Instance;
class Light;
class Mesh;
struct RaytracingContext;
struct Triangle;
struct Vertex;

struct BakeParams
{
    Eigen::Vector3f skyColor;

    uint32_t lightBounceCount {};
    uint32_t lightSampleCount {};

    uint32_t shadowSampleCount {};
    float shadowSearchArea {};
};

struct BakePoint
{
    const Mesh* mesh {};
    const Triangle* triangle {};
    Eigen::Vector2f uv;

    uint16_t x {}, y {};

    Eigen::Vector3f color;
    float shadow {};

    Vertex getInterpolatedVertex() const;
};

enum PaintMode
{
    PAINT_MODE_COLOR = 1,
    PAINT_MODE_SHADOW = 2,
    PAINT_MODE_COLOR_AND_SHADOW = PAINT_MODE_COLOR | PAINT_MODE_SHADOW
};

class BakingFactory
{
public:
    static std::vector<BakePoint> createBakePoints(const Instance& instance, uint16_t width, uint16_t height);

    static void paintBitmap(const Bitmap& bitmap, const std::vector<BakePoint>& bakePoints, PaintMode mode);

    static std::unique_ptr<Bitmap> createAndPaintBitmap(const std::vector<BakePoint>& bakePoints, uint16_t width,
                                                        uint16_t height, PaintMode mode);

    static Eigen::Vector3f pathTrace(const RaytracingContext& raytracingContext, const Eigen::Vector3f& position,
                                     const Eigen::Vector3f& direction, const Light& sunLight,
                                     const BakeParams& bakeParams);

    static void bake(const RaytracingContext& raytracingContext, std::vector<BakePoint>& bakePoints,
                     const BakeParams& bakeParams);
};
