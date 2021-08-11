#include "Instance.h"
#include "Mesh.h"

void Instance::buildAABB()
{
    aabb.setEmpty();

    for (auto& mesh : meshes)
        aabb.extend(mesh->aabb);
}