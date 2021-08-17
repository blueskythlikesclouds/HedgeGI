#pragma once

class Scene;
class Light;

class LightBVH
{
    class Node
    {
    public:
        AABB aabb;
        const Light* light {};
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    std::unique_ptr<Node> node;
    const Light* sunLight {};
    
    static std::unique_ptr<Node> build(const std::vector<const Light*>& lights);

    template<typename T>
    void traverse(const Vector3& position, const T& callback, const Node& node) const
    {
        if (!node.aabb.contains(position)) return;

        if (node.light) callback(node.light);
        if (node.left) traverse(position, callback, *node.left);
        if (node.right) traverse(position, callback, *node.right);
    }

    template<int N>
    void traverse(const Vector3& position, std::array<const Light*, N>& lights, size_t& lightCount, const Node& node) const
    {
        if (lightCount == N || !node.aabb.contains(position)) return;

        if (node.light) lights[lightCount++] = node.light;
        if (node.left) traverse(position, lights, lightCount, *node.left);
        if (node.right) traverse(position, lights, lightCount, *node.right);
    }

public:
    LightBVH();
    ~LightBVH();

    bool valid() const;

    void reset();
    void build(const Scene& scene);

    template<typename T>
    void traverse(const Vector3& position, const T& callback) const
    {
        if (node) traverse(position, callback, *node);
        if (sunLight) callback(sunLight);
    }

    template<int N>
    void traverse(const Vector3& position, std::array<const Light*, N>& lights, size_t& lightCount) const
    {
        if (sunLight) lights[lightCount++] = sunLight;
        if (node) traverse(position, lights, lightCount, *node);
    }
};
