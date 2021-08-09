#include "LightBVH.h"
#include "Light.h"
#include "Scene.h"

class LightBVH::Node
{
public:
    AABB aabb;
    const Light* light {};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

std::unique_ptr<LightBVH::Node> LightBVH::build(const std::vector<const Light*>& lights)
{
    if (lights.empty())
        return nullptr;

    AABB aabb;

    for (auto& item : lights)
    {
        const float range = item->range.w();
        const Vector3 vec3(range, range, range);

        aabb.extend(item->position - vec3);
        aabb.extend(item->position + vec3);
    }

    const Eigen::Vector3f center = aabb.center();

    std::unique_ptr<Node> node = std::make_unique<Node>();
    node->aabb = aabb;

    if (lights.size() == 1)
    {
        node->light = lights[0];
        return node;
    }

    std::vector<const Light*> left;
    std::vector<const Light*> right;

    size_t dimIndex;
    aabb.sizes().maxCoeff(&dimIndex);

    for (auto& item : lights)
    {
        if (item->position[dimIndex] < center[dimIndex])
            left.push_back(item);
        else
            right.push_back(item);
    }

    if (left.empty())
        std::swap(left, right);

    if (right.empty())
    {
        for (size_t i = 0; i < left.size(); i++)
        {
            if ((i & 1) == 0)
                continue;

            right.push_back(left.back());
            left.pop_back();
        }
    }

    node->left = build(left);
    node->right = build(right);

    if (node->left != nullptr && node->right == nullptr)
        return std::move(node->left);

    if (node->left == nullptr && node->right != nullptr)
        return std::move(node->right);

    return node;
}

void LightBVH::traverse(const Vector3& position, std::function<void(const Light*)>& callback, const Node& node) const
{
    if (!node.aabb.contains(position)) return;

    if (node.light) callback(node.light);
    if (node.left) traverse(position, callback, *node.left);
    if (node.right) traverse(position, callback, *node.right);
}

LightBVH::LightBVH() = default;
LightBVH::~LightBVH() = default;

bool LightBVH::valid() const
{
    return sunLight || node;
}

void LightBVH::reset()
{
    sunLight = nullptr;
    node = nullptr;
}

void LightBVH::build(const Scene& scene)
{
    sunLight = nullptr;

    std::vector<const Light*> lights;

    for (auto& light : scene.lights)
    {
        if (light->type == LightType::Directional)
            sunLight = light.get();

        else if (light->type == LightType::Point)
            lights.push_back(light.get());
    }

    node = build(lights);
}

void LightBVH::traverse(const Vector3& position, std::function<void(const Light*)> callback) const
{
    if (node) traverse(position, callback, *node);
    if (sunLight) callback(sunLight);
}
