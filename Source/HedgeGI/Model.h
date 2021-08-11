#pragma once

class Mesh;

class Model
{
public:
    std::string name;
    std::vector<const Mesh*> meshes;
};
