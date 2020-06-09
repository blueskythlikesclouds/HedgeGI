#pragma once

class Mesh;
class Scene;

class Instance
{
public:
    AxisAlignedBoundingBox aabb;

    std::string name;
    std::vector<const Mesh*> meshes;

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};
