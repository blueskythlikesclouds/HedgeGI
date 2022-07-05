#pragma once

class Bitmap;
class Instance;
class Mesh;

struct Triangle;
struct Vertex;

struct SeamNode
{
    float key;
    uint32_t meshIndex;
    uint32_t triangleIndex;
};

class SeamOptimizer
{
    const Instance& instance;
    std::list<SeamNode> nodes;

    static void blend(size_t stepCount, const Vector2& startA, const Vector2& endA, const Vector2& startB, const Vector2& endB, const Bitmap& bitmap);
    static void compareAndBlend(const Mesh& mA, const Mesh& mB, const Triangle& tA, const Triangle& tB, const Bitmap& bitmap);
    static size_t computeStepCount(const Vector2& p1, const Vector2& p2, size_t width, size_t height);
    static int32_t findVertex(const Vector3& position, const Vector3& normal, const Vertex& a, const Vertex& b, const Vertex& c);
public:
    SeamOptimizer(const Instance& instance);
    ~SeamOptimizer();

    std::unique_ptr<Bitmap> optimize(const Bitmap& bitmap) const;
};
