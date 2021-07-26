#pragma once

class FileStream;
class Mesh;
class Scene;

class Instance
{
public:
    std::string name;
    std::vector<const Mesh*> meshes;
    AABB aabb;

    void buildAABB();

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};
