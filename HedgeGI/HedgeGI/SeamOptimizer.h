#pragma once

class Bitmap;
class Instance;

class SeamOptimizer
{
    const Instance& instance;

    static void blend(uint32_t stepCount, const Eigen::Vector2f& startA, const Eigen::Vector2f& endA, const Eigen::Vector2f& startB, const Eigen::Vector2f& endB, const Bitmap& bitmap);
    static void compareAndBlend(const Mesh& mA, const Mesh& mB, const Triangle& tA, const Triangle& tB, const Bitmap& bitmap);
    static uint32_t computeStepCount(const Eigen::Vector2f& p1, const Eigen::Vector2f& p2, uint32_t width, uint32_t height);
    static int32_t findVertex(const Eigen::Vector3f& position, const Eigen::Vector3f& normal, const Vertex& a, const Vertex& b, const Vertex& c);
public:
    SeamOptimizer(const Instance& instance);
    ~SeamOptimizer();

    std::unique_ptr<Bitmap> optimize(const Bitmap& bitmap) const;
};
