#pragma once

class Scene;
class Light;

class LightBVH
{
    class Node;

    std::unique_ptr<Node> node;
    const Light* sunLight {};
    
    static std::unique_ptr<Node> build(const std::vector<const Light*>& lights);
    void traverse(const Vector3& position, std::function<void(const Light*)>& callback, const Node& node) const;

public:
    LightBVH();
    ~LightBVH();

    bool valid() const;

    void reset();
    void build(const Scene& scene); 
    void traverse(const Vector3& position, std::function<void(const Light*)> callback) const;
};
