#include "Instance.h"

#include "FileStream.h"
#include "Mesh.h"
#include "Scene.h"

void Instance::buildAABB()
{
    aabb.setEmpty();

    for (auto& mesh : meshes)
        aabb.extend(mesh->aabb);
}

void Instance::read(const FileStream& file, const Scene& scene)
{
    name = file.readString();

    const uint32_t count = file.read<uint32_t>();

    meshes.reserve(count);
    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t index = file.read<uint32_t>();
        if (index != (uint32_t)-1)
            meshes.push_back(scene.meshes[index].get());
    }

    aabb = file.read<AABB>();
}

void Instance::write(const FileStream& file, const Scene& scene) const
{
    file.write(name);
    file.write((uint32_t)meshes.size());

    for (auto& mesh : meshes)
    {
        uint32_t index = (uint32_t)-1;

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            if (mesh != scene.meshes[i].get())
                continue;

            index = (uint32_t)i;
            break;
        }

        file.write((uint32_t)index);
    }

    file.write(aabb);
}
