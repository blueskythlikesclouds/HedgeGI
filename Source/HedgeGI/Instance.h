#pragma once

class Mesh;

class Instance
{
public:
    std::string name;
    std::vector<const Mesh*> meshes;
    AABB aabb;

    void buildAABB();
};
