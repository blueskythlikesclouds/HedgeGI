#include "Material.h"
#include "Scene.h"

void Material::read(const FileStream& file, const Scene& scene)
{
    name = file.readString();

    uint32_t index = file.read<uint32_t>();
    if (index != (uint32_t)-1)
        diffuse = scene.bitmaps[index].get();

    index = file.read<uint32_t>();
    if (index != (uint32_t)-1)
        emission = scene.bitmaps[index].get();
}

void Material::write(const FileStream& file, const Scene& scene) const
{
    file.write(name);

    uint32_t index = (uint32_t)-1;
    for (size_t i = 0; i < scene.bitmaps.size(); i++)
    {
        if (diffuse != scene.bitmaps[i].get())
            continue;

        index = (uint32_t)i;
        break;
    }

    file.write(index);

    index = (uint32_t)-1;
    for (size_t i = 0; i < scene.bitmaps.size(); i++)
    {
        if (emission != scene.bitmaps[i].get())
            continue;

        index = (uint32_t)i;
        break;
    }

    file.write(index);
}
